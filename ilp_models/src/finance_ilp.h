#ifndef FINANCE_ILP_H
#define FINANCE_ILP_H

#include "cm_options.h"
#include "bn_engine.hpp"
#include "gurobi_c++.h"

class financial_info {
public:
    int initial_wealth;
    int target_capital;
    int unit_benefit;
    int unit_penalty;
    int maximum_difference;
    int num_stages;
    vector< vector< double > > return_rates;
};

class FinanceModelILP {
public:
    FinanceModelILP(GRBEnv* _env,
                     BNEngine& bn0, Cm_opt& opts
                    );

private:
    BNEngine& bn;
    financial_info data;
    GRBEnv* env;
    GRBModel model;
    vector< GRBVar > util_vars;
    vector< double > util_coefs;

    vector< int > min_wealth;
    vector< int > max_wealth;

    vector<string> var_names;
    unordered_map<string, int> var_ids;
    const vector<vector<int> >* val_ids;
    size_t num_stages;
    void create_scenario(vector<pair<int, int> >& assignments,
                         vector<GRBVar>& dvars,
			 int loc);
    void process_scenario(vector<pair<int, int> >& assignments,
                          vector< GRBVar >& scenario_vars);
    void read_financial_data(string data_filename,
                          financial_info & data);
    void get_wealth_bounds(const financial_info&,
			vector< int >&,
			vector< int >&);
};

#endif //FINANCE_ILP_H
