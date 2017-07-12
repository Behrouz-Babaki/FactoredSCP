#pragma GCC diagnostic ignored "-Wunused-local-typedefs"

#include "knapsack_ground.h"
#include <vector>
#include <utility>
#include <iostream>
#include <algorithm>
#include <gecode/int.hh>
#include <gecode/float.hh>
#include "time_util.hpp"
#include <cstdlib>

using Gecode::linear;
using Gecode::IRT_GQ;
using Gecode::IRT_EQ;
using Gecode::IRT_LQ;
using Gecode::FRT_EQ;
using Gecode::IntVarArgs;
using Gecode::IntArgs;
using Gecode::FloatValArgs;
using Gecode::FloatVarArgs;
using Gecode::channel;
using Gecode::IntVarArray;
using Gecode::FloatVarArray;
using Gecode::branch;
using Gecode::INT_VAR_MERIT_MIN;
using Gecode::INT_VAL_MIN;
using Gecode::IntSharedArray;
using Gecode::SharedArray;

using std::pair;
using std::make_pair;
using std::cout;
using std::endl;
using std::to_string;
using std::min_element;
using std::max_element;

KnapsackGModel::KnapsackGModel(BNEngine& bn0, Cm_opt& opts) :
    capacity(opts.capacity) ,bn(bn0), num_dec_vars(0)
{
    var_ids = bn.get_var_ids();
    val_ids = bn.get_val_ids();
    num_stages = var_ids.size() / 3;

    var_names.resize(var_ids.size());
    for (auto i : var_ids)
        var_names[i.second] = i.first;


    // iterate through the worlds, print the assignments and joint probability
    vector<pair<int, int> > assignments;
    vector<IntVar> dvars;
    vector<int> scenario_indices;
    vector<int> util_indices;
    vector<double> util_coefs;

    double grounding_start_cpu = get_cpu_time();
    create_scenario(assignments,
                    dvars,
                    scenario_indices,
                    util_indices,
                    util_coefs,
                    0);
    // print grounding time
    cout << "grounding time (cpu): " << get_cpu_time() - grounding_start_cpu << endl;    
    cout << "number of variables: " << num_dec_vars << endl;
    assert(num_dec_vars == dvars.size());
    
    decs_coef_array = SharedArray<double>(dvars.size());
    for (int i=0; i!=decs_coef_array.size(); i++)
      decs_coef_array[i] = 0;

    for (size_t i=0; i!=util_indices.size(); i++)
      decs_coef_array[util_indices[i]] += util_coefs[i];

    decs_var_array = IntVarArray(*this, IntVarArgs(dvars));
    decs_fvar_array = FloatVarArray(*this, decs_var_array.size());
    size_t i=0;
    for (IntVar v : decs_var_array) {
        FloatVar fv(*this, v.min(), v.max());
        channel(*this, fv, v);
        decs_fvar_array[i++] = fv;
    }

    util = FloatVar(*this, Gecode::Float::Limits::min, Gecode::Float::Limits::max);
	FloatValArgs cons_coefs(decs_coef_array.begin(), decs_coef_array.end());
    linear(*this, cons_coefs, decs_fvar_array, FRT_EQ, util);

    branch(*this, decs_var_array, INT_VAR_MERIT_MIN(&merit), INT_VAL_MIN());

}


void KnapsackGModel::print(void) const
{
    double exputil = 0;
    for (auto x : decs_var_array) cout << x.val() << " "; cout << endl;
    for (int i=0; i!=decs_var_array.size(); i++) {
        exputil += decs_coef_array[i] * decs_var_array[i].val();
    }
    cout << exputil << endl;
}
// for GIST (if enabled)
void KnapsackGModel::print(std::ostream& os) const
{
    os << decs_var_array << std::endl;
    os << util << std::endl;
}


void KnapsackGModel::create_scenario(vector<pair<int, int> >& assignments,
                                 vector<IntVar>& dvars,
                                 vector<int>& scenario_indices,
                                 vector<int>& util_indices,
                                 vector<double>& util_coefs,
                                 int loc) {

    if (loc == (int) num_stages) {
        vector<double> coefs;
        process_scenario(assignments, dvars, scenario_indices, coefs);
        util_indices.insert(util_indices.end(), scenario_indices.begin(), scenario_indices.end());
        util_coefs.insert(util_coefs.end(), coefs.begin(), coefs.end());
        return;
    }
    
    if (assignments.size()%2 == 0) {
      	// create the decision variable
	IntVar current_dvar(*this, 0, 1);
	num_dec_vars++;
	dvars.push_back(current_dvar);
	
	string var_name = "w" + to_string(loc);
	int varid = var_ids.at(var_name);
	scenario_indices.push_back(dvars.size()-1);
	for(int val : val_ids->at(varid)) {
	    assignments.push_back(make_pair(varid, val));
	    create_scenario(assignments, dvars,
			    scenario_indices,
			    util_indices,
			    util_coefs,
			    loc);
	    assignments.pop_back();
	}
	scenario_indices.pop_back();
    }
    else {
	string var_name = "r" + to_string(loc);
	int varid = var_ids.at(var_name);
	for(int val : val_ids->at(varid)) {
	    assignments.push_back(make_pair(varid, val));
	    create_scenario(assignments, dvars,
			    scenario_indices,
			    util_indices,
			    util_coefs,
			    loc+1);
	    assignments.pop_back();
	}
    }
    return;
}

void KnapsackGModel::process_scenario(vector<pair<int, int> >& assignments,
                                  vector< IntVar >& dvars,
                                  vector< int >& scenario_indices,
                                  vector< double >& coefs) {
    // decision variables that correspond to this world
    IntVarArgs scenario_dvars;
    for(auto i : scenario_indices)
        scenario_dvars << dvars[i];
    
    // constraints:  \sum_i w_i X_i <= capacity
    IntArgs weights;
    for(size_t i=0; i!=num_stages; i++)
      weights << assignments[2*i].second;
    linear(*this, weights, scenario_dvars, IRT_LQ, capacity);


    // contribution to exputil :  pr * (\sum_i r_i * X_i)
    double p = bn.pr(assignments);
    coefs.clear();
    for (size_t i=0; i!=num_stages; i++) 
        coefs.push_back(p * assignments[(2*i)+1].second);
}

static double merit(const Space& home, IntVar x, int i) {
  return i;
}
