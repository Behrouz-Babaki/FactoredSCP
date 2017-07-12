#pragma once

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
using Gecode::Space;
using Gecode::FloatVar;
using Gecode::FloatVarArgs;
using Gecode::FloatValArgs;
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

class Inv2GModel : public Space {
protected:
    FloatVar util;
    IntVarArray decs_var_array;
    FloatVarArray decs_fvar_array;
    SharedArray<FloatNum> decs_coef_array;
    int capacity;

private:
    BNEngine& bn;
    vector<string> var_names;
    unordered_map<string, int> var_ids;
    const vector<vector<int> >* val_ids;
    size_t num_stages;
    unsigned long int num_dec_vars;

public:
    Inv2GModel(BNEngine&, Cm_opt&);
    Inv2GModel(bool share, Inv2GModel& s) :
        Space(share, s) , bn(s.bn) {
        util.update(*this, share, s.util);
        decs_var_array.update(*this, share, s.decs_var_array);
        decs_fvar_array.update(*this, share, s.decs_fvar_array);
        decs_coef_array.update(*this, share, s.decs_coef_array);
    }
    virtual Space* copy(bool share) {
        return new Inv2GModel(share, *this);
    }
    void print(void) const;
    void print(std::ostream&) const;

    virtual void constrain(const Space& _best) {
        const Inv2GModel& best = static_cast<const Inv2GModel&> (_best);
        rel(*this, util, FRT_GR, best.util.max());
    }
    
    double solution_value(void){
      	double value = 0;
        for (int i=0; i!=decs_fvar_array.size(); i++) 
            value += decs_coef_array[i] * decs_var_array[i].val();
	return value;
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
                          vector< double >& coefs);
};

static double merit(const Space&, IntVar, int);
