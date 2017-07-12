#ifndef BOOk_ILP_H
#define BOOK_ILP_H

#include "cm_options.h"
#include "bn_engine.hpp"
#include "gurobi_c++.h"


class BookModelILP {
public:
    BookModelILP(GRBEnv* _env,
                     BNEngine& bn0, Cm_opt& opts
                    );

private:
    BNEngine& bn;
    GRBEnv* env;
    GRBModel model;
    double util_constant;


    vector<string> var_names;
    unordered_map<string, int> var_ids;
    const vector<vector<int> >* val_ids;
    int num_stages;
    long int scenario_counter;
    double update_time;
    double ground_start;
    void create_scenario(vector<pair<int, int> >& assignments,
                         vector<GRBVar>& dvars,
                         vector<int>& scenario_indices,
                         vector<int>& util_indices,
                         vector<double>& util_coefs,
                         int loc);
    void process_scenario(vector<pair<int, int> >& assignments,
                          vector< GRBVar >& dvars,
                          vector< int >& scenario_indices,
                          vector< double >& coefs,
			  double& constant);
    void post_constraint(vector< pair < int, int > >& assignments,
				  vector< GRBVar>& dvars, 
				  vector< int >& scenario_indices);
    void update_model(void);
};

#endif //BOOK_ILP_H
