#ifndef BOOK_GROUND_H
#define BOOK_GROUND_H

#include "bn_engine.hpp"
#include "cm_options.h"
#include <vector>
#include <utility>
#include <map>
#include <iostream>
#include <gecode/minimodel.hh>
#include <gecode/float.hh>

class Cm_opt;
using std::vector;
using std::pair;
using std::unordered_map;
using std::cout;
using std::endl;

using Gecode::Space;
using Gecode::FloatMaximizeSpace;
using Gecode::FloatVar;
using Gecode::FloatVarArgs;
using Gecode::FloatValArgs;
using Gecode::FloatVal;
using Gecode::IntVar;
using Gecode::IntVarArray;
using Gecode::IntArgs;
using Gecode::IntVarArgs;
using Gecode::FloatNum;
using Gecode::SharedArray;
using Gecode::IntSharedArray;
using Gecode::FRT_GR;
using Gecode::rel;
using Gecode::FloatVarArray;
using Gecode::linear;

class BookGModel : public FloatMaximizeSpace {
protected:
    FloatVar util;
    IntVarArray decs_var_array;
    FloatVarArray decs_fvar_array;
    SharedArray<FloatNum> decs_coef_array;
    double util_constant;

private:
    BNEngine& bn;
    vector<string> var_names;
    unordered_map<string, int> var_ids;
    const vector<vector<int> >* val_ids;
    int num_stages;
    unsigned long int num_dec_vars;

public:
    BookGModel(BNEngine&, Cm_opt&);
    BookGModel(bool share, BookGModel& s) :
        FloatMaximizeSpace(share, s) , bn(s.bn) {
        util_constant = s.util_constant;
	util.update(*this, share, s.util);
        decs_var_array.update(*this, share, s.decs_var_array);
        decs_fvar_array.update(*this, share, s.decs_fvar_array);
        decs_coef_array.update(*this, share, s.decs_coef_array);
    }
    virtual Space* copy(bool share) {
        return new BookGModel(share, *this);
    }
    void print(void) const;


    virtual void constrain(const Space& _best) {
        const BookGModel& best = static_cast<const BookGModel&> (_best);
	rel(*this, util, FRT_GR, best.util.max());
    }

    
    virtual FloatVar cost(void) const {
      return util;
    }
    
    double solution_value() {
      return util.max() + util_constant;
    }

private:
    void create_scenario(vector<pair<int, int> >& assignments,
                         vector<IntVar>& dvars,
                         vector<int>& scenario_indices,
                         vector<int>& util_indices,
                         vector<double>& util_coefs,
                         int loc);
    void process_scenario(vector<pair<int, int> >& assignments,
                          vector< IntVar >& dvars,
                          vector< int >& scenario_indices,
                          vector< double >& coefs,
                          double& constant);
    void post_constraint(vector< pair< int, int > >& assignments,
			 vector< IntVar>& dvars, 
			 vector< int >& scenario_indices);
};

static double merit(const Space&, IntVar, int);

#endif // BOOK_GROUND_H
