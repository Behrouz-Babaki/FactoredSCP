#include <iostream>
#include <sstream>
#include <algorithm>

#include "book_ilp.h"
#include "cm_options.h"
#include "time_util.hpp"

using namespace std;


BookModelILP::BookModelILP(GRBEnv* _env,
                                   BNEngine& bn0, Cm_opt& opts):
    bn(bn0), env(_env), model(GRBModel(*env)), util_constant(0) {


    var_ids = bn.get_var_ids();
    val_ids = bn.get_val_ids();
    num_stages = var_ids.size() / 2;

    var_names.resize(var_ids.size());
    for (auto i : var_ids)
        var_names[i.second] = i.first;


    vector<pair<int, int> > assignments;
    vector<GRBVar> dvars;
    vector<int> scenario_indices;
    vector<int> util_indices;
    vector<double> util_coefs;

    model.set(GRB_StringAttr_ModelName, "book");
    double grounding_start_cpu = get_cpu_time();
    create_scenario(assignments,
                    dvars,
                    scenario_indices,
                    util_indices,
                    util_coefs,
                    0);

    
    
    vector<double> coefs(dvars.size(), 0);
    for(size_t i=0; i<util_indices.size(); i++)
        coefs[util_indices[i]] += util_coefs[i];

    GRBLinExpr obj_expr;
    obj_expr.addTerms(coefs.data(), dvars.data(), dvars.size());
    model.setObjective(obj_expr, GRB_MAXIMIZE);
    model.update();
    
    // print grounding time
    cout << "grounding time (cpu): " << get_cpu_time() - grounding_start_cpu << endl;    
    
    // supress the output
    model.getEnv().set(GRB_IntParam_OutputFlag, opts.verbose);

    // Use barrier to solve root relaxation
    model.getEnv().set(GRB_IntParam_Method, GRB_METHOD_BARRIER);
    
    cout << "number of variables: " << model.get(GRB_IntAttr_NumVars) << endl;
    cout << "number of integer variables: " << model.get(GRB_IntAttr_NumIntVars) << endl;

    // Solve
    model.optimize();
    int status = GRB_OPTIMAL;
    if (status == 2)
      cout << "model solved to optimality" << endl;
    else if (status == 9)
      cout << "solving stopped (timeout)"<< endl;

    // Print solution and runtime
    cout << "\nexputil: " << model.get(GRB_DoubleAttr_ObjVal) + util_constant << endl;
    cout << "solving time: " << model.get(GRB_DoubleAttr_Runtime) << endl;

}




void BookModelILP::create_scenario(vector<pair<int, int> >& assignments,
                                 vector<GRBVar>& dvars,
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
    int min_val = *min_element(val_ids->at(varid).begin(), val_ids->at(varid).end());
    int max_val = *max_element(val_ids->at(varid).begin(), val_ids->at(varid).end());
    GRBVar current_dvar = model.addVar(min_val, max_val, 0, GRB_INTEGER);
    // model.update();

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

void BookModelILP::post_constraint(vector< pair < int, int > >& assignments,
				  vector< GRBVar >& dvars, 
				  vector< int >& scenario_indices){

  // decision variables that correspond to this world
    vector< GRBVar > scenario_dvars;
    for(auto i : scenario_indices)
        scenario_dvars.push_back(dvars[i]); 
    
    // constraints:  \sum_j p_j >= \sum_j d_j  
    int demand_sum = 0;
    for (auto a : assignments)  
        demand_sum += a.second;
    vector<double> weights(scenario_dvars.size(), 1);
     GRBLinExpr cons_expr;
     cons_expr.addTerms(weights.data(), scenario_dvars.data(), scenario_dvars.size());
     model.addConstr(cons_expr, GRB_GREATER_EQUAL, demand_sum);
     // model.update();    
    
}

void BookModelILP::process_scenario(vector<pair<int, int> >& assignments,
                                  vector< GRBVar >& dvars,
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
