#ifndef KNAPSACK_ILP_H
#define KNAPSACK_ILP_H

#include "cm_options.h"
#include "bn_engine.hpp"
#include "gurobi_c++.h"

class KnapsackModelILP {
public:
    KnapsackModelILP(GRBEnv* _env,
                     BNEngine& bn0, Cm_opt& opts
                    );

private:
    BNEngine& bn;
    GRBEnv* env;
    GRBModel model;
    double capacity;
    double ground_start;
    double update_time;

    vector<string> var_names;
    unordered_map<string, int> var_ids;
    const vector<vector<int> >* val_ids;
    size_t num_stages;
    long int scenario_counter;
    void create_scenario(vector<pair<int, int> >& assignments,
                         vector<GRBVar>& dvars,
                         vector<int>& scenario_indices,
                         vector<int>& util_indices,
                         vector<double>& util_coefs,
                         int loc);
    void process_scenario(vector<pair<int, int> >& assignments,
                          vector< GRBVar >& dvars,
                          vector< int >& scenario_indices,
                          vector< double >& coefs);
    void update_model(void);
};

#endif //KNAPSACK_ILP_H
