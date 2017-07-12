#pragma once

#include <iostream>
#include <cassert>

#include "bn_engine.hpp"

using std::cout;
using std::endl;

class AceEngine : public BNEngine {
public:
    // get domain values of random variables
    virtual const vector<vector<int> >* get_val_ids();
    virtual size_t domain_size(int);

    // get value,prob pairs of all values of variable_index given the evidence
    virtual vector< pair<int,double> > var_partials(const vector< pair< int, int > >& evidence, int variable);
    virtual const vector<double>& _var_partials(const vector< pair< int, int > >& new_evidence, int variable) ; // raw, uncached
    // get probability of evidence
    virtual double pr(const vector< pair< int, int > >& evidence);
    virtual double pr(const vector< pair< int, int > >& evidence, int next_var, int next_val); // more efficient then the above

private:
    virtual void query(vector<int>&, vector<int>&, 
		       vector<int>&, int, 
		       vector<double>&) = 0;  
};

inline
const vector< vector< int > >* AceEngine::get_val_ids()
{
  // pre-fetched in get_value_map();
  return &this->bn_val_ids;
}

inline
size_t AceEngine::domain_size(int var_id)
{
  // pre-fetched in get_value_map();
  return this->bn_val_ids[var_id].size();
}

inline
double AceEngine::pr(const vector<pair<int, int> >& evidence){
  if (verbose >= 5) {
    cout << "pr()\n";
    cout << "Evidence:\n";
    for (size_t i=0; i!=evidence.size(); i++)
      cout << " " << evidence[i].first << "," << evidence[i].second;
    cout << "\n";
  }
  
  if (evidence.size() == 0) {
    // empty, so probably 1 but use cache anyway (can't do pop_back trick)
    const vector<double>& probs = _var_partials(evidence, 0);
    double pr = 0.0;
    for (auto& v: probs)
      pr += v;
    return pr;
  }
  
  vector< pair<int,int> > par_evidence(evidence);
  par_evidence.pop_back(); // remove parent from par_evidence
  return pr(par_evidence, evidence.back().first, evidence.back().second); // evidence, par_var, par_val
}

inline
double AceEngine::pr(const vector<pair<int, int> >& evidence, int par_var, int par_val) {
  // get all probs of parent and extract the one of this child
  const vector<double>& probs = _var_partials(evidence, par_var);
  return probs[bn_val_map[par_var][par_val]];
}

inline
vector<pair<int, double> > AceEngine::var_partials(const vector<pair<int, int> >& evidence, int variable){
  if (verbose >= 5) {
    cout << "var_partials for var idx: " << variable << "\n";
    cout << "Evidence:\n";
    for (size_t i=0; i!=evidence.size(); i++)
      cout << " " << evidence[i].first << "," << evidence[i].second;
    cout << "\n";
  }
  
  const vector<double>& probs = _var_partials(evidence, variable);
  
  // convert to (value,prob) instead of (id,prob)
  const vector<int>& order = this->bn_val_ids[variable];
  if (order.size() != probs.size())
    cout << "var="<<variable<<" order.size="<<order.size()<<" != "<<probs.size()<<" probs.size()\n";
  assert(order.size() == probs.size());
  vector< pair<int,double> > ret(probs.size());
  for (size_t i=0; i!=order.size(); i++) {
    ret[i].first = order[i];
    ret[i].second = probs[i];
  }
  return ret;
}

// find the commit and retract var/vals
inline
const vector<double>& AceEngine::_var_partials(const vector< pair< int, int > >& evidence, int variable){
  
  vector<double>& lookup = cache_partials[evidence];
  if (lookup.size() == 0) {
    // cache miss, create
    
    vector<int> commit_vars, commit_vals, retract_vars;
    // New: no need to retract+commit same var: commit overwites previous commit
    // (and remember: evidence is always in exactly the same order: varBNorder)
    
    // overwrite new values of already committed vars
    size_t sizeboth = std::min(evidence.size(), ac_evidence.size());
    for (size_t i=0; i!=sizeboth; i++) {
      if (evidence[i].second != ac_evidence[i].second) {
        commit_vars.push_back(evidence[i].first);
        commit_vals.push_back(bn_val_map[evidence[i].first][evidence[i].second]);
        if (verbose >= 5)
          cout << "commit var="<<evidence[i].first<<" val_dom="<<evidence[i].second<<" val_idx="<<bn_val_map[evidence[i].first][evidence[i].second]<<"\n";
      }
    }
    // add new values of new vars
    for (size_t i=sizeboth; i!=evidence.size(); i++) {
      commit_vars.push_back(evidence[i].first);
      commit_vals.push_back(bn_val_map[evidence[i].first][evidence[i].second]);
      if (verbose >= 5)
        cout << "commit var="<<evidence[i].first<<" val_dom="<<evidence[i].second<<" val_idx="<<bn_val_map[evidence[i].first][evidence[i].second]<<"\n";
    }
    // retract old vars
    for (size_t i=sizeboth; i!=ac_evidence.size(); i++) {
      retract_vars.push_back(ac_evidence[i].first);
      if (verbose >= 5)
        cout << "retract var="<<ac_evidence[i].first<<" val_dom="<<ac_evidence[i].second<<"\n";
    }
    
    // store new evidence
    ac_evidence = evidence;
    
    // query inference engine
    query(commit_vars, commit_vals, retract_vars, variable, lookup);
  } // end of cache miss, create
  
  return lookup;
}
