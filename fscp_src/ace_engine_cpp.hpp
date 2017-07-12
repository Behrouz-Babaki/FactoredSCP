#pragma once

#include <set>
#include <boost/lexical_cast.hpp>

using std::set;

#include "ace_engine.hpp"
#include "AceEvalCpp.hpp"

inline int get_int_name(string name){
  int start = 1;
  int coef = 1;
    if (name.at(1) == '_') {
      start = 2;
      coef = -1;
   }
    return coef * boost::lexical_cast<int>(name.substr(start));
}

class AceEngineCpp : public AceEngine{
 public:
  AceEngineCpp (string, string, int cache_level=1, int verbosity=0);
  // get names:ids of random variables 
  virtual const unordered_map<string,int> get_var_ids();
  virtual int num_vars();
  
 private:
  virtual void query(vector<int>&, vector<int>&,vector<int>&, 
             int, vector<double>&);
  OnlineEngine engine;
  Evidence evidence;
  vector<Variable> variables;
};


inline AceEngineCpp::AceEngineCpp(string ac_filename, string lm_filename, int cache_level, int verbosity) : 
  engine(OnlineEngine(ac_filename, lm_filename)), evidence(engine)
{
  this->set_verbose(verbosity);
  this->set_cache_level(cache_level);
  
  for (auto var : engine.variables()){
    variables.push_back(var);
    vector<string> domain_names = var.domainNames();
    vector<int> domain_values;
    unordered_map<int, int> val_map;
    int j=0;
    for (auto name : domain_names) {
      int domain_value = get_int_name(name);
      domain_values.push_back(domain_value);
      val_map[domain_value] = j++;
    }
    bn_val_ids.push_back(domain_values);
    bn_val_map.push_back(val_map);
  }
}

inline int AceEngineCpp::num_vars()
{
  return variables.size();
}

inline const unordered_map< string, int > AceEngineCpp::get_var_ids()
{
  unordered_map<string, int> bn_var_names;
  for (int i=0; i<num_vars(); i++)
    bn_var_names[variables[i].name()] = i;
  return bn_var_names;
}

inline void AceEngineCpp::query(vector< int >& commit_vars, vector< int >& commit_vals,
				vector< int >& retract_vars, int variable_index, vector< double >& lookup)
{
  for (int i=0, s=commit_vars.size(); i < s; i++)
    evidence.varCommit(variables[commit_vars[i]], commit_vals[i]);
  for (auto r_var : retract_vars)
    evidence.varRetract(variables[r_var]);
  engine.assertEvidence(evidence, true);
  Variable var = variables[variable_index];
  // XXX this should better be (const) Variable& var = ... to avoid copy
  lookup = engine.varPartials(var);
}


