#pragma GCC diagnostic ignored "-Wunused-local-typedefs"

#include <vector>
#include <iomanip>
#include <gecode/float.hh>
#include <gecode/int.hh>
#include <gecode/search.hh>
#include <cstdlib>

#include "book_model.h"
#include "constraint_andall.hpp"
#include "brancher_exputil.hpp"

using std::vector;
using std::cout;
using std::cerr;
using std::endl;
using std::ostream;
using std::setprecision;
using std::to_string;

using namespace Gecode;

BookModel::BookModel(PolTreeState& poltree, Cm_opt& opts) :
  vars(*this, poltree.bn.num_vars()), // uninitialised
  util(*this, Int::Limits::min, Int::Limits::max)
{
  // get names (ids) of random variables and their domain values
  const unordered_map<string,int>& bn_var_names = poltree.bn.get_var_ids();
  const vector< vector<int> >* bn_val_ids = poltree.bn.get_val_ids();
  size_t numStages = bn_val_ids->size() / 2;
  
  // set order, save BN identifiers: -1=decision var, else bn variable index
  vector<int> varBNid;
  // here, 1 dec 1 rand 1 dec etc... random variables in BN file ordered according to stages
  for (size_t s=0; s!=numStages; s++) {
    string name = "d"+to_string(s); // 'd0': first stage, 'd1': second, ...
    int bn_var_id = bn_var_names.at(name);
    
    // prep domain, some domain for rand and dec here
    int domain_size = bn_val_ids->at(bn_var_id).size();
    int val_ids[domain_size];
    std::copy(bn_val_ids->at(bn_var_id).begin(),
              bn_val_ids->at(bn_var_id).end(),
              val_ids);
    IntSet rand_dom(val_ids, domain_size);
    
    // ??? Should the decision variables have the same domain as random variables?
    
    // OR node (decision variable)
    vars[s*2] = IntVar(*this, rand_dom);
    varBNid.push_back(-1); // not a rand var
    
    // AND node (random variable)
    vars[(s*2)+1] = IntVar(*this, rand_dom);
    varBNid.push_back(bn_var_id); // id according to BN
  }
  poltree.init_bndata(varBNid);
  
  // constraint 0:
  // and nodes may not have there domain pruned (it will select out the ANDs)
  and_nodes_all(*this, vars, poltree);
  
  // constraint 1:
  // util = n(d_1 - p_1) + (n-1)(d_2 - p_2) + ... + (d_n - p_n)
  // negated because maximisation problem
  IntArgs varUtilCoef;
  for (size_t i=0; i != numStages; i++){
    varUtilCoef << -(numStages-i); // OR var (d)
    varUtilCoef << (numStages-i); // AND var (p)
  }
  linear(*this, varUtilCoef, vars, IRT_EQ, util);
  
  // for exputil
  // now manually and store it in BN (C++11 lambda function, yeuy!)
  poltree.max_f = [numStages](vector< pair<int,int> >& VS){
    assert(VS.size() == numStages * 2); // V1,S1,V2,S2
    // return n V1.max - n S1.min + ... + Vnmax - Sn.min
    int bound = 0;
    for (size_t i=0; i != numStages; i++){
      bound -= (numStages-i)*VS[(i*2)].first;
      bound += (numStages-i)*VS[(i*2)+1].second;
    }
    return bound;
  };
  
  // constraint 2:
  // overstock in each stage must be >= 0
  for (size_t s=0; s!=numStages; s++) {
    IntArgs overstCoef;
    IntVarArgs overstVar;
    for (size_t i=0; i<=s; i++) {
      // 1 decis, 1 rand var per stage
      overstVar << vars[i*2]; overstCoef << 1;
      overstVar << vars[(i*2)+1]; overstCoef << -1;
    }
    linear(*this, overstCoef, overstVar, IRT_GQ, 0);
  }
  
  
  //* branching(Space, vars, poltree, order_or_max, order_and_max, depth_or, depth_and);
  bool order_or_max = false; // default
  if (opts.branch_or == 1)
    order_or_max = true;
  bool order_and_max = true; // default
  if (opts.branch_and == -1)
    order_and_max = false;
  branch_exputil(*this, vars, poltree, order_or_max, order_and_max, opts.depth_or, opts.depth_and);
}
