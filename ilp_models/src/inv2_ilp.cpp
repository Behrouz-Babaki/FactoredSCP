#include <iostream>
#include <sstream>

#include "inv2_ilp.h"
#include "cm_options.h"
#include "time_util.hpp"

using namespace std;


Inv2ModelILP::Inv2ModelILP(GRBEnv* _env,
                           BNEngine& bn0,
                           Cm_opt& opts):
    bn(bn0), env(_env), model(GRBModel(*env)) {

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

    model.set(GRB_StringAttr_ModelName, "investment 2");

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


void Inv2ModelILP::create_scenario(vector<pair<int, int> >& assignments,
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
        //create two variables for two kinds of investment
        GRBVar current_x = model.addVar(0, 1, 0, GRB_BINARY);
        dvars.push_back(current_x);
        scenario_indices.push_back(dvars.size()-1);

        GRBVar current_y = model.addVar(0, 1, 0, GRB_BINARY);
        dvars.push_back(current_y);
        scenario_indices.push_back(dvars.size()-1);

        model.addConstr(current_x + current_y == 1);

        string var_name = "a" + to_string(loc);
        int varid = var_ids.at(var_name);
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
        scenario_indices.pop_back();

    }
    else {

        string var_name = "b" + to_string(loc);
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

void Inv2ModelILP::process_scenario(vector<pair<int, int> >& assignments,
                                    vector< GRBVar >& dvars,
                                    vector< int >& scenario_indices,
                                    vector< double >& coefs) {
  	  vector< GRBVar > scenario_xvars;
	  vector< GRBVar > scenario_yvars;
	  
	  for(size_t i=0; i!=num_stages; i++) {
	      int x_index = scenario_indices[2*i];
	      int y_index = scenario_indices[2*i+1];
	      scenario_xvars.push_back(dvars[x_index]);
	      scenario_yvars.push_back(dvars[y_index]);
	  }

	  vector< double > x_coefs, y_coefs;
	  for (size_t i=0; i!=num_stages; i++){
	    x_coefs.push_back(assignments[2*i].second);
	    y_coefs.push_back(assignments[2*i+1].second);
	  }
	  GRBLinExpr x_expr, y_expr;
	  x_expr.addTerms(x_coefs.data(), scenario_xvars.data(), num_stages);
	  y_expr.addTerms(y_coefs.data(), scenario_yvars.data(), num_stages);
	  model.addConstr(y_expr, GRB_LESS_EQUAL, x_expr);
	  
    // contribution to exputil :  pr * \sum_i (a_i * X_i + b_i * Y_i)
    double p = bn.pr(assignments);
    coefs.clear();
    for (auto assignment : assignments)
        coefs.push_back(p * assignment.second);
}
