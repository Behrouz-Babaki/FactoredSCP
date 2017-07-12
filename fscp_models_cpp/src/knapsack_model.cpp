#pragma GCC diagnostic ignored "-Wunused-local-typedefs"

#include <vector>
#include <iomanip>
#include <gecode/float.hh>
#include <gecode/int.hh>
#include <gecode/search.hh>
#include <cstdlib>

#include "knapsack_model.h"
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

KnapsackModel::KnapsackModel(PolTreeState& poltree, Cm_opt& opts) :
    vars(*this, poltree.bn.num_vars()), // uninitialised
    util(*this, Int::Limits::min, Int::Limits::max)
{
    // get names (ids) of random variables and their domain values
    const unordered_map<string,int>& bn_var_names = poltree.bn.get_var_ids();
    const vector< vector<int> >* bn_val_ids = poltree.bn.get_val_ids();
    size_t numStages = bn_val_ids->size() / 3;

    // set order, save BN identifiers: -1=decision var, else bn variable index
    vector<int> varBNid;
    // here, 1 dec 1 rand 1 dec etc... random variables in BN file ordered according to stages
    for (size_t s=0; s!=numStages; s++) {
         // OR node (decision variable)
        vars[s*3] = IntVar(*this, 0, 1);
        varBNid.push_back(-1); // not a rand var
	
        char letters[2][2] = {"w", "r"};
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
            vars[(s*3)+i+1] = IntVar(*this, rand_dom);
            varBNid.push_back(bn_var_id); // id according to BN
        }
    }
    
    poltree.init_bndata(varBNid);

    // constraint 0:
    // and nodes may not have there domain pruned (it will select out the ANDs)
    and_nodes_all(*this, vars, poltree);

    // constraint 1:
    // util = r_1 * x_1 + ... + r_n * x_n
    // \sum_i v[3i+2]*v[3i]

    IntVarArgs rxs;
    for (size_t i=0; i != numStages; i++) {
        IntVar rx (*this, 0, vars[(3*i)+2].max());
        mult(*this, vars[(3*i)+2], vars[3*i], rx);
        rxs << rx;
    }
    linear(*this, rxs, IRT_EQ, util);

    // for exputil
    // now manually and store it in BN (C++11 lambda function, yeuy!)
    poltree.max_f = [numStages](vector< pair< int, int> >& VS) {
        int bound = 0;
        for (size_t i=0; i!=numStages; i++)
            bound += VS[3*i+2].second * VS[3*i].second;
        return bound;
    };

    // constraint 2:
    // sum of weights must be smaller than capacity
    // \sum_i v[3i+1]*v[3i] <= capacity
    IntVarArgs wxs;
    for (size_t i=0; i!=numStages; i++) {
        IntVar wx(*this, 0, vars[3*i+1].max());
        mult(*this, vars[3*i+1], vars[3*i], wx);
        wxs << wx;
    }
    //cout << "capacity:" << opts.capacity << endl;
    linear(*this, wxs, IRT_LQ, opts.capacity);

    //* branching(Space, vars, poltree, order_or_max, order_and_max, depth_or, depth_and);
    bool order_or_max = true; // default
    if (opts.branch_or == -1)
        order_or_max = false;
    bool order_and_max = true; // default
    if (opts.branch_and == -1)
        order_and_max = false;
    branch_exputil(*this, vars, poltree, order_or_max, order_and_max, opts.depth_or, opts.depth_and);
}

