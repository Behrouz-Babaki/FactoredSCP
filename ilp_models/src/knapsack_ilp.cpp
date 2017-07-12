#include <iostream>
#include <sstream>

#include "knapsack_ilp.h"
#include "cm_options.h"
#include "time_util.hpp"

using namespace std;


KnapsackModelILP::KnapsackModelILP(GRBEnv* _env,
                                   BNEngine& bn0, Cm_opt& opts):
    bn(bn0), env(_env), model(GRBModel(*env)), capacity(opts.capacity) {


    var_ids = bn.get_var_ids();
    val_ids = bn.get_val_ids();
    num_stages = var_ids.size() / 3;

    var_names.resize(var_ids.size());
    for (auto i : var_ids)
        var_names[i.second] = i.first;


    vector<pair<int, int> > assignments;
    vector<GRBVar> dvars;
    vector<int> scenario_indices;
    vector<int> util_indices;
    vector<double> util_coefs;

    model.set(GRB_StringAttr_ModelName, "knapsack");

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
    cout << "number of binary variables: " << model.get(GRB_IntAttr_NumBinVars) << endl;    

    // Solve
    model.optimize();
    int status = GRB_OPTIMAL;
    if (status == 2)
      cout << "model solved to optimality" << endl;
    else if (status == 9)
      cout << "solving stopped (timeout)"<< endl;
    
    // Print solution and runtime
    cout << "\nexputil: " << model.get(GRB_DoubleAttr_ObjVal) << endl;
    cout << "solving time: " << model.get(GRB_DoubleAttr_Runtime) << endl;

}


void KnapsackModelILP::create_scenario(vector<pair<int, int> >& assignments,
                                       vector<GRBVar>& dvars,
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
        GRBVar current_dvar = model.addVar(0, 1, 0, GRB_BINARY);
        //model.update();
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

void KnapsackModelILP::process_scenario(vector<pair<int, int> >& assignments,
                                        vector< GRBVar >& dvars,
                                        vector< int >& scenario_indices,
                                        vector< double >& coefs) {

    // decision variables that correspond to this world
    vector< GRBVar > scenario_dvars;
    for(auto i : scenario_indices)
        scenario_dvars.push_back(dvars[i]);

    // constraints:  \sum_i w_i X_i <= capacity
    vector< double > weights;
    for(size_t i=0; i!=num_stages; i++)
        weights.push_back(assignments[2*i].second);
    GRBLinExpr cons_expr;
    cons_expr.addTerms(weights.data(), scenario_dvars.data(), weights.size());
    model.addConstr(cons_expr, GRB_LESS_EQUAL, capacity);
    //model.update();



    // contribution to exputil :  pr * (\sum_i r_i * X_i)
    double p = bn.pr(assignments);
    coefs.clear();
    for (size_t i=0; i!=num_stages; i++)
        coefs.push_back(p * assignments[(2*i)+1].second);
}
