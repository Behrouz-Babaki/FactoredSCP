#pragma once

#include <vector>
#include <string>
#include <utility>
#include <functional>
#include <gecode/int.hh>
#include <ace_engine.hpp>

using std::vector;
using std::string;
using std::pair;

class PolTreeState {
 public:
  PolTreeState (AceEngine& _bn,
		int verbose0=0)
  : verbose(verbose0), depth(0), bn(_bn){}
  ~PolTreeState() {}
  
  void init_bndata(vector<int>& varBNid);
  double bound_or(const Gecode::ViewArray<Gecode::Int::IntView>& vars, int pos, int depth_limit);
  void bounds_and(const Gecode::ViewArray<Gecode::Int::IntView>& vars, int pos, vector< std::pair<int,double> >& vals, int depth_limit);
  void new_leaf(const Gecode::IntVarArray& vars, const Gecode::IntVar& util);
  double max_f_vars(const Gecode::IntVarArray& vars);
  
 public:
  double min_inf = -std::numeric_limits<double>::max();
  int verbose;
  
  // stores state of the policy tree (not full tree needed)
  int depth; // first unassigned position (initially 0)
  std::vector<double> evals; // (best-so-far) value of node at depth in policy tree
  std::vector<double> lb; // lower bound of node at that depth
  
  AceEngine& bn;
  std::vector<int> varBNid; // set with init_bndata()
  
  // for random variables (detect failure)
  vector<int> brch_rand_val; // which value the brancher chose for this rand var, -1=none
  vector<bool> brch_rand_lastval; // this value is the last value to try for this var
  
  // for the utility
  std::function<int(vector< pair<int,int> >&)> max_f; // function, must be set in CP model!
  
 private:
  vector< pair<int,int> > varsmima; // overwrite at will, trick to avoid memory-allocating it each time
  vector<pair<int, int> > leaf_evidence; // same reason as varsmima
  double _bound_dfs(vector< pair< int, int > >& varsmima, vector< pair< int, int > >& evidence, int pos, int depth_limit);
  double _bound_dfs_loop(vector< pair< int, int > >& varsmima, vector< pair< int, int > >& evidence, int pos, int depth_limit, const Gecode::ViewArray< Gecode::Int::IntView >& vars);
};

