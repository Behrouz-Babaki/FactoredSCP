#include <iostream>
#include <sstream>
#include <algorithm>

#include "finance_ilp.h"
#include "cm_options.h"

using namespace std;


FinanceModelILP::FinanceModelILP(GRBEnv* _env,
                                 BNEngine& bn0, 
				 Cm_opt& opts):
    bn(bn0), env(_env), model(GRBModel(*env)) {

    read_financial_data(opts.data_file, data);
    get_wealth_bounds(data, min_wealth, max_wealth);
    var_ids = bn.get_var_ids();
    val_ids = bn.get_val_ids();
    num_stages = var_ids.size() / 3;

    var_names.resize(var_ids.size());
    for (auto i : var_ids)
        var_names[i.second] = i.first;


    vector<pair<int, int> > assignments;
    vector<GRBVar> scenario_vars;

    model.set(GRB_StringAttr_ModelName, "finance");

    create_scenario(assignments,
                    scenario_vars,
                    0);

    GRBLinExpr obj_expr;
    obj_expr.addTerms(util_coefs.data(), util_vars.data(), util_vars.size());
    model.setObjective(obj_expr, GRB_MAXIMIZE);
    model.update();

    // Use barrier to solve root relaxation
    model.getEnv().set(GRB_IntParam_Method, GRB_METHOD_BARRIER);
    
    // supress output
    model.getEnv().set(GRB_IntParam_OutputFlag, 0);

    // Solve
    model.optimize();

    // Print solution
    cout << "\nexpected utility: " << model.get(GRB_DoubleAttr_ObjVal) << endl;

}


void FinanceModelILP::create_scenario(vector<pair<int, int> >& assignments,
                                      vector<GRBVar>& scenario_vars,
                                      int loc) {

    if (loc == (int) num_stages) {
        process_scenario(assignments, scenario_vars);
        return;
    }

    if (assignments.size()%2 == 0) {
        // decision variables for investment
        for (size_t i=0; i<2; i++) {
            GRBVar inv = model.addVar(0, max_wealth[loc], 0, GRB_INTEGER);
            model.update();
            scenario_vars.push_back(inv);
        }
        // decision variable for wealth
        GRBVar wealth = model.addVar(min_wealth[loc], max_wealth[loc], 0, GRB_INTEGER);
        scenario_vars.push_back(wealth);
        model.update();

        string var_name = "a" + to_string(loc);
        int varid = var_ids.at(var_name);
        for(int val : val_ids->at(varid)) {
            assignments.push_back(make_pair(varid, val));
            create_scenario(assignments, 
			    scenario_vars,
                            loc);
            assignments.pop_back();
        }
        // pop the decision variables
        for (size_t i=0; i<3; i++)
	  scenario_vars.pop_back();
    }
    else {

        string var_name = "b" + to_string(loc);
        int varid = var_ids.at(var_name);
        for(int val : val_ids->at(varid)) {
            assignments.push_back(make_pair(varid, val));
            create_scenario(assignments, 
                            scenario_vars,
                            loc+1);
            assignments.pop_back();
        }
    }
    return;
}

void FinanceModelILP::process_scenario(vector<pair<int, int> >& assignments,
                                       vector< GRBVar >& scenario_vars) {
    // decision variables that correspond to this world
    vector< GRBVar > inv_1, inv_2, w;
    size_t i=0;
    while (i < scenario_vars.size()) {
        inv_1.push_back(scenario_vars[i++]);
        inv_2.push_back(scenario_vars[i++]);
        w.push_back(scenario_vars[i++]);
    }
    
    // constraint: inv_1 + inv_2 = wealth
    model.addConstr(inv_1[0] + inv_2[0], GRB_EQUAL,  data.initial_wealth);
    model.update();
    for (size_t i=1; i!=num_stages; i++) {
        model.addConstr(inv_1[i] + inv_2[i], GRB_LESS_EQUAL, w[i-1]);
        model.update();
    }

    // constraints:  w_i = r1 * inv1 + r2 * inv2
    for(size_t i=0; i!=num_stages; i++) {
        double rate1 = data.return_rates[0][assignments[2*i].second];
        double rate2 = data.return_rates[1][assignments[2*i+1].second];
        model.addConstr(rate1 * inv_1[i] + rate2 * inv_2[i], GRB_LESS_EQUAL, w[i]);
        model.update();
    }

    // constraint: wealth - surplus + shortage = target
    GRBVar shortage = model.addVar(0, max(0, data.target_capital - min_wealth.back()), 0, GRB_CONTINUOUS);
    model.update();
    GRBVar surplus = model.addVar(0, max_wealth.back() - data.target_capital, 0, GRB_CONTINUOUS);
    model.update();
    model.addConstr(w.back() - surplus + shortage, GRB_EQUAL, data.target_capital);
    model.update();
    
    // constraint: \sum (inv1 - inv2) <= max_difference
    GRBLinExpr diff_expr;
    vector< GRBVar> diff_expr_vars;
    vector< double > diff_expr_coefs;
    diff_expr_vars.insert(diff_expr_vars.end(),
			  inv_1.begin(), inv_1.end());
    diff_expr_coefs.insert(diff_expr_coefs.end(),
			   num_stages, 1);
    diff_expr_vars.insert(diff_expr_vars.end(),
			  inv_2.begin(), inv_2.end());
    diff_expr_coefs.insert(diff_expr_coefs.end(),
			   num_stages, -1);
    diff_expr.addTerms(diff_expr_coefs.data(), diff_expr_vars.data(), num_stages*2);
    
    model.addConstr(diff_expr, GRB_LESS_EQUAL, data.maximum_difference);
    model.update();
    
    // constraint: \sum (inv1 - inv2) >= -max_difference
    model.addConstr(diff_expr, GRB_GREATER_EQUAL, -data.maximum_difference);
    model.update();

    double p = bn.pr(assignments);
    util_vars.push_back(shortage);
    util_coefs.push_back(-p * data.unit_penalty);

    util_vars.push_back(surplus);
    util_coefs.push_back(p * data.unit_benefit);

}

void FinanceModelILP::read_financial_data(string data_filename,
financial_info& data) {
    ifstream data_file(data_filename);
    data_file >> data.initial_wealth
    >> data.target_capital
    >> data.unit_benefit
    >> data.unit_penalty
    >> data.maximum_difference
    >> data.num_stages;
    vector< vector< double > > rates_v;
    double rate;
    while (data_file >> rate) {
        vector< double > rates;
        while (rate > 0) {
            rates.push_back(rate);
            data_file >> rate;
        }
        rates_v.push_back(rates);
    }
    data.return_rates = rates_v;
}

void FinanceModelILP::get_wealth_bounds(const financial_info& data,
                                        vector< int >& min_wealth,
vector< int >& max_wealth) {
    double max_rate = 0;
    double min_rate = data.return_rates[0][0];
    for (auto rv : data.return_rates) {
        max_rate = max(max_rate, *max_element(rv.begin(), rv.end()));
        min_rate = min(min_rate, *min_element(rv.begin(), rv.end()));
    }

    min_wealth.clear();
    max_wealth.clear();
    int wealth_high = data.initial_wealth;
    int wealth_low = data.initial_wealth;
    for (int s=0; s!=data.num_stages; s++) {
        wealth_high += int(wealth_high * max_rate);
        wealth_low += int(wealth_low * min_rate);
	max_wealth.push_back(wealth_high);
        min_wealth.push_back(wealth_low);
    }
    
}
