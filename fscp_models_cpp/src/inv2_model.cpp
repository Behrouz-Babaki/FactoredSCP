#pragma GCC diagnostic ignored "-Wunused-local-typedefs"

#include <vector>
#include <iomanip>
#include <gecode/float.hh>
#include <gecode/int.hh>
#include <gecode/search.hh>
#include <cstdlib>

#include "inv2_model.h"
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

Inv2Model::Inv2Model(PolTreeState& poltree, Cm_opt& opts) :
    vars(*this, poltree.bn.num_vars()*4/3), // uninitialised
    util(*this, Int::Limits::min, Int::Limits::max)
{
    // get names (ids) of random variables and their domain values
    const unordered_map<string,int>& bn_var_names = poltree.bn.get_var_ids();
    const vector< vector<int> >* bn_val_ids = poltree.bn.get_val_ids();
    size_t numStages = bn_val_ids->size() / 3;

    // set order, save BN identifiers: -1=decision var, else bn variable index
    vector<int> varBNid;
    // here, 2 dec 2 rand 2 dec etc... random variables in BN file ordered according to stages
    for (size_t s=0; s!=numStages; s++) {
         // OR node (decision variable)
        vars[s*4] = IntVar(*this, 0, 1);
        varBNid.push_back(-1); // not a rand var
	vars[s*4+1] = IntVar(*this, 0, 1);
	varBNid.push_back(-1);
	
        char letters[2][2] = {"a", "b"};
        for (int i=0; i<2; i++) {
            string name = letters[i] + to_string(s); // 'w0': first weight, 'r0': first reward, ...
            int bn_var_id = bn_var_names.at(name);

            // prep domain, some domain for rand and dec here
            int domain_size = bn_val_ids->at(bn_var_id).size();
            int val_ids[domain_size];
            std::copy(bn_val_ids->at(bn_var_id).begin(),
                      bn_val_ids->at(bn_var_id).end(),
                      val_ids);
            IntSet rand_dom(val_ids, domain_size);
            // AND node (random variable)
            vars[(s*4)+2+i] = IntVar(*this, rand_dom);
            varBNid.push_back(bn_var_id); // id according to BN
        }
    }
    
    poltree.init_bndata(varBNid);

    // constraint 0:
    // and nodes may not have there domain pruned (it will select out the ANDs)
    and_nodes_all(*this, vars, poltree);

    // constraint 1:
    // util = \sum_i (a_i * x_i + b_i * y_i)
    // \sum_i v[4i+2]*v[4i] + v[4i+3]*v[4i+1]

    IntVarArgs rxs;
    for (size_t i=0; i != numStages; i++) 
      for(size_t j=0; j!=2; j++) {
        IntVar rx (*this, 0, vars[(4*i)+2+j].max());
        mult(*this, vars[(4*i)+2+j], vars[4*i+j], rx);
        rxs << rx;
    }
    linear(*this, rxs, IRT_EQ, util);

    // for exputil
    // now manually and store it in BN (C++11 lambda function, yeuy!)
    poltree.max_f = [numStages](vector< pair< int, int> >& VS) {
        int bound = 0;
        for (size_t i=0; i!=numStages; i++)
            bound += VS[4*i+2].second * VS[4*i].second + 
		     VS[4*i+3].second * VS[4*i+1].second;
        return bound;
    };
    
    
    // constraint 2:
    // x_i xor y_i
    for (size_t i=0; i!=numStages; i++)
      rel(*this, vars[4*i], IRT_NQ, vars[4*i+1]);


    IntVarArgs xs, ys, as, bs;
    for (size_t i=0; i!=numStages; i++) {
	xs << vars[4*i];
	ys << vars[4*i+1];
	as << vars[4*i+2];
	bs << vars[4*i+3];
    }
    
  
    // constraint 3
    // \sum_i b_i * y_i <= sum_i a_i * x_i for all k
    
    IntVarArgs axs, bys;
    for (size_t i=0; i != numStages; i++) 
    {
        IntVar ax (*this, 0, as[i].max());
        mult(*this, xs[i], as[i], ax);
        axs << ax;
	
        IntVar by (*this, 0, bs[i].max());
        mult(*this, ys[i], bs[i], by);
        bys << by;
    }
    IntVar ax_sum(*this, Int::Limits::min, Int::Limits::max);  
    linear(*this, axs, IRT_EQ, ax_sum);
    IntVar by_sum(*this, Int::Limits::min, Int::Limits::max);
    linear(*this, bys, IRT_EQ, by_sum);
    rel(*this, by_sum, IRT_LQ, ax_sum);
    
   
    //* branching(Space, vars, poltree, order_or_max, order_and_max, depth_or, depth_and);
    bool order_or_max = true; // default
    if (opts.branch_or == -1)
        order_or_max = false;
    bool order_and_max = true; // default
    if (opts.branch_and == -1)
        order_and_max = false;
    branch_exputil(*this, vars, poltree, order_or_max, order_and_max, opts.depth_or, opts.depth_and);
}

