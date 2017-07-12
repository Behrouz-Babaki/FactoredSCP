  #pragma GCC diagnostic ignored "-Wunused-local-typedefs"

#include "book_ground.h"
#include <vector>
#include <utility>
#include <iostream>
#include <algorithm>
#include <gecode/float.hh>
#include <cstdlib>

#include "time_util.hpp"

using Gecode::linear;
using Gecode::IRT_GQ;
using Gecode::IRT_EQ;
using Gecode::FRT_EQ;
using Gecode::IntVarArgs;
using Gecode::IntArgs;
using Gecode::FloatValArgs;
using Gecode::FloatVarArgs;
using Gecode::channel;
using Gecode::IntVarArray;
using Gecode::FloatVarArray;
using Gecode::branch;
using Gecode::INT_VAR_NONE;
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

BookGModel::BookGModel(BNEngine& bn0, Cm_opt& opts) :
    util_constant(0), bn(bn0), num_dec_vars(0)
{
    var_ids = bn.get_var_ids();
    val_ids = bn.get_val_ids();
    num_stages = var_ids.size() / 2;

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

    //TODO simplify
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


void BookGModel::print(void) const {
    cout << util.max() + util_constant << endl;
}


void BookGModel::create_scenario(vector<pair<int, int> >& assignments,
                                 vector<IntVar>& dvars,
                                 vector<int>& scenario_indices,
                                 vector<int>& util_indices,
                                 vector<double>& util_coefs,
                                 int loc) {

    if (loc == num_stages) {
        vector<double> coefs;
        double constant;
        process_scenario(assignments, dvars, scenario_indices, coefs, constant);
        util_indices.insert(util_indices.end(), scenario_indices.begin(), scenario_indices.end());
        util_coefs.insert(util_coefs.end(), coefs.begin(), coefs.end());
        util_constant += constant;
        return;
    }

    string var_name = "d" + to_string(loc);
    int varid = var_ids.at(var_name);

    // create the production variable
    IntVar current_dvar(*this,
                        *min_element(val_ids->at(varid).begin(), val_ids->at(varid).end()),
                        *max_element(val_ids->at(varid).begin(), val_ids->at(varid).end()));
    num_dec_vars++;

    dvars.push_back(current_dvar);
    scenario_indices.push_back(dvars.size()-1);
    for(int val : val_ids->at(varid)) {
        assignments.push_back(make_pair(varid, val));
	post_constraint(assignments, dvars,
			scenario_indices);
        create_scenario(assignments, dvars,
                        scenario_indices,
                        util_indices,
                        util_coefs,
                        loc+1);
        assignments.pop_back();
    }
    scenario_indices.pop_back();
    return;
}

void BookGModel::post_constraint(vector< pair < int, int > >& assignments,
				  vector< IntVar>& dvars, 
				  vector< int >& scenario_indices){
    // decision variables that correspond to this world
    IntVarArgs scenario_dvars;
    for(auto i : scenario_indices)
        scenario_dvars << dvars[i]; 
    
    // constraints:  \sum_j p_j >= \sum_j d_j  
    int demand_sum = 0;
    for (auto a : assignments)  
        demand_sum += a.second;
    linear(*this, scenario_dvars, IRT_GQ, demand_sum);
    
}

void BookGModel::process_scenario(vector<pair<int, int> >& assignments,
                                  vector< IntVar >& dvars,
                                  vector< int >& scenario_indices,
                                  vector< double >& coefs,
                                  double& constant) {
    
    /*
     * contribution to exputil :  pr * (- \sum_i \sum_j (p_j - d_j))
     * pr * [n(d_0 - p_0) + (n-1)(d_1 - p_1) + ... + (d_n - p_n)]
     * vars: p_0, p_1, ... , p_n
     * coefs: -pr*n, -pr*(n-1), ... -pr
     * constant: pr * (nd_0 + (n-1)d_1 + ... + d_n)
    */
    double p = bn.pr(assignments);
    constant = 0;
    coefs.clear();
    for (int i=0; i!=num_stages; i++) {
        coefs.push_back(-p * (num_stages - i));
        constant += (num_stages - i) * assignments[i].second;
    }
    constant *= p;
}

static double merit(const Space& home, IntVar x, int i) {
  return i;
}
