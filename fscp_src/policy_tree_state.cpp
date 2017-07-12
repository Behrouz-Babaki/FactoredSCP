
#include "policy_tree_state.h"

using namespace std;
using namespace Gecode;


void PolTreeState::init_bndata(vector< int >& varBNid0)
{
  if (verbose >= 5) {
    for (size_t i=0; i!=varBNid0.size(); i++)
      cout << "varBNid["<<i<<"] = " << varBNid0[i] << "\n";
  }
  
  // an id (or -1) for each variable/depth in the order
  varBNid = varBNid0;
  size_t nr_vars = varBNid.size();
  varsmima.resize(nr_vars);
  
  // initialize leaf_evidence
  for (size_t i=0; i!=nr_vars; i++) {
    int bn_id = varBNid[i];
    if (bn_id != -1) // AND node
      leaf_evidence.push_back( make_pair(bn_id, -1) );
  }
  
  // valuation of the exputil for each variable/depth
  evals.resize(nr_vars); // will be initialised in brancher
  lb.resize(nr_vars, min_inf);
  brch_rand_val.resize(nr_vars, -1); // will only be used for rand vars (varBnid[i] != -1)
  brch_rand_lastval.resize(nr_vars, false); // will only be used for rand vars (varBnid[i] != -1)
}


double PolTreeState::bound_or(const ViewArray< Int::IntView >& vars, int pos, int depth_limit)
{
  // init the input for the prob and the input for the utility
  int and_size = 0;
  int and_pos = 0;
  for (int i=0; i!=vars.size(); i++) {
    varsmima[i].first = vars[i].min();
    varsmima[i].second = vars[i].max();
    if (varBNid[i] != -1) {
      and_size += 1;
      if (i < pos) // assigned AND node
        and_pos++;
    }
  }
  // some nerdy code to do 'one shot' memory allocation
  vector< pair<int,int> > evidence;
  evidence.reserve(and_size);
  evidence.resize(and_pos);
  int s = 0;
  for (int i=0; i!=pos; i++) {
    if (varBNid[i] != -1) {
      evidence[s].first = varBNid[i];
      evidence[s].second = vars[i].val();
      s++;
    }
  }
  depth_limit = min(depth_limit, and_size-and_pos);
  
  if (verbose >= 4) {
    cout << "Init bound_or (depth_limit="<<depth_limit<<"):\n";
    cout << "evidence:";
    for (size_t i=0; i!=evidence.size(); i++)
      cout << " " << evidence[i].first << "," << evidence[i].second;
    cout << "\n";
    cout << "varsmima:";
    for (size_t i=0; i!=varsmima.size(); i++)
      cout << " " << varsmima[i].first << "," << varsmima[i].second;
    cout << "\n";
  }
  
  if (depth_limit == 0) {
    // naive bound
    double prob = bn.pr(evidence);
    int util = max_f(varsmima);
    
    if (verbose >= 3)
      cout << "Naive bound: "<<util*prob<<" ( "<<util<<" * "<<prob<<" )\n";
    return util*prob;
    
  } else { // depth_limit bound
    
    return _bound_dfs(varsmima, evidence, pos, depth_limit);
    //return _bound_dfs_loop(varsmima, evidence, pos, depth_limit, vars); // slower
  }
}

// vals is <val,prob> vector
// we replace it with a <val,exputil_ub> vector
void
PolTreeState::bounds_and(const ViewArray< Int::IntView >& vars,
                         int pos,
                         vector< pair< int, double > >& return_vals,
                         int depth_limit
                        )
{
  // init the input for the utility
  int and_size = 0;
  int evd_size = 0;
  for (int i=0; i!=vars.size(); i++) {
    varsmima[i].first = vars[i].min();
    varsmima[i].second = vars[i].max();
    if (varBNid[i] != -1) {
      and_size += 1;
      evd_size += (i < pos);
    }
  }
  depth_limit = min(depth_limit, and_size-evd_size);
  
  if (verbose >= 4) {
    cout << "Init bound_and_simple:\n";
    cout << "varsmima:";
    for (size_t i=0; i!=varsmima.size(); i++)
      cout << " " << varsmima[i].first << "," << varsmima[i].second;
    cout << "\n";
  }
  
  // replace return_vals' second from prob to ub on exputil
  if (depth_limit <= 1) {
    for (size_t i=0; i!=return_vals.size(); i++) {
      int val = return_vals[i].first;
      double prob = return_vals[i].second;
      varsmima[pos].first = val; varsmima[pos].second = val;
      int util = max_f(varsmima);
      if (verbose >= 5)
        cout << "vars["<<pos<<"]="<<val<<": util*prob = "<<util*prob<<" = "<<util<<" * "<<prob<<"\n";
    
      return_vals[i].second = util*prob;
    }
  } else {
    // do DFS to depth_limit (>1)
    vector< pair<int,int> > evidence;
    for (int i=0; i!=pos; i++) {
      if (varBNid[i] != -1)
        evidence.push_back( make_pair(varBNid[i], varsmima[i].first) ); // guaranteed assigned
    }
    size_t s = evidence.size();
    evidence.push_back( make_pair(varBNid[pos], -1) ); // random value, will be overwritten
    for (size_t i=0; i!=return_vals.size(); i++) {
      int val = return_vals[i].first;
      varsmima[pos].first = val; varsmima[pos].second = val;
      evidence[s].second = val;
      return_vals[i].second = _bound_dfs(varsmima, evidence, pos+1, depth_limit-1);
    }
  }
}

double PolTreeState::_bound_dfs(vector< pair<int,int> >& varsmima,
                                vector< pair<int,int> >& evidence,
                                int pos,
                                int depth_limit)
{
  while (varBNid[pos] == -1)
    pos++;
  if (verbose >= 4) {
    cout << "_bound_dfs(vmm,ev,"<<pos<<","<<depth_limit<<")\n";
    cout << "varsmima, depth="<<depth<<":";
    for (size_t i=0; i!=varsmima.size(); i++)
      cout << " " << varsmima[i].first << "," << varsmima[i].second;
    cout << "\n";
    cout << "evidence:";
    for (size_t i=0; i!=evidence.size(); i++)
      cout << " " << evidence[i].first << "," << evidence[i].second;
    cout << "\n";
  }
  assert(depth_limit > 0);
  
  if (depth_limit == 1) {
    double v = 0;
    const vector< pair<int,double> >& probs = bn.var_partials(evidence, varBNid[pos]);
    for (size_t i=0; i!=probs.size(); i++) {
      int val = probs[i].first;
      double prob = probs[i].second;
      varsmima[pos].first = val; varsmima[pos].second = val;
      int util = max_f(varsmima);
      if (verbose >= 5)
        cout << "Leaf! vars["<<pos<<"]="<<val<<": util*prob = "<<util*prob<<" = "<<util<<" * "<<prob<<"\n";
      v += util*prob;
    }
    return v;
  }
  
  // else: dive down
  double v = 0;
  size_t s = evidence.size();
  evidence.push_back( make_pair(varBNid[pos], -1) ); // random val, will be overwritten
  const vector< vector<int> >* val_ids = bn.get_val_ids();
  for (auto val : val_ids->at(varBNid[pos])) {
    varsmima[pos].first = val; varsmima[pos].second = val;
    evidence[s].second = val;
    v += _bound_dfs(varsmima, evidence, pos+1, depth_limit-1);
  }
  evidence.pop_back();
  return v;
}

// same DFS bound, but using a loop instead of recursion
double PolTreeState::_bound_dfs_loop(vector< pair<int,int> >& varsmima,
                                     vector< pair<int,int> >& evidence,
                                     int pos,
                                     int depth_limit,
                                     const ViewArray< Int::IntView >& vars // TODO: remove
)
{  
  // begin of DFS iterations over AND nodes, start from first unassigned var
  double bnd = 0.0;
  int ev_size = evidence.size();
  const vector< vector<int> >* val_ids = bn.get_val_ids();
  // dfs state, values will be set next
  int d = 0;
  evidence.resize(ev_size + depth_limit-1); // not last one, we use var_partials on that
  vector<int> dfs_pos(depth_limit);
  vector<int> dfs_idx(depth_limit, 0); // dfs_idx[d] = 0..dfs_size[d]-1
  vector<int> dfs_sizem1(depth_limit); // difference offset 0 and offset 1
  for (size_t i=pos; i!=varsmima.size(); i++) {
    int bn_id = varBNid[i];
    if (bn_id != -1) {
      dfs_pos[d] = i;
      dfs_sizem1[d] = val_ids->at(bn_id).size() - 1;
      if (d == depth_limit-1)
        break; // not last one, we use var_partials on that
      evidence[ev_size+d].first = bn_id;
      evidence[ev_size+d].second = varsmima[i].first;  // smallest value
      varsmima[i].second = varsmima[i].first;
      d++;
    }
  }
  assert(d == depth_limit-1); // just checking assumptions
  // will start at final depth, everything ready for that leaf
  
  while (true) {
    if (verbose >= 4) {
      cout << "DFS_loop, depth="<<d<<" (limit="<<depth_limit<<")\n";
      cout << "  varsmima:";
      for (size_t i=0; i!=varsmima.size(); i++)
        cout << " " << varsmima[i].first << "," << varsmima[i].second;
      cout << "\n";
      cout << "  evidence:";
      for (size_t i=0; i!=evidence.size(); i++)
        cout << " " << evidence[i].first << "," << evidence[i].second;
      cout << "\n";
    }
    if (d == depth_limit-1) { // d is offset 0, depth_limit is offset 1
      // get exputil of last layer
      int i = dfs_pos[d];
      const vector< pair<int,double> >& probs = bn.var_partials(evidence, varBNid[i]);
      for (size_t j=0; j!=probs.size(); j++) {
        int val = probs[j].first;
        double prob = probs[j].second;
        varsmima[i].first = val; varsmima[i].second = val;
        int util = max_f(varsmima);
        if (verbose >= 5)
          cout << "Leaf! vars["<<i<<"]="<<val<<": util*prob = "<<util*prob<<" = "<<util<<" * "<<prob<<"\n";
        bnd += util*prob;
      }
      
      // backtracking
      d--;
      while (d >= 0 && dfs_idx[d] == dfs_sizem1[d]) {
        // up to first non-last-value depth
        dfs_idx[d] = -1; // because ++ on dive down
        d--;
      }
      if (d < 0) // stopping criterion
        break; 
      //cout << "DFS, continuing with d="<<d<<" and val=0 (-1 now)\n";
    }
    
    // dive down
    int i = dfs_pos[d];
    dfs_idx[d]++;
    int val = val_ids->at(varBNid[i])[dfs_idx[d]];
    evidence[ev_size+d].second = val;
    varsmima[i].first = val; varsmima[i].second = val;
    d++;
    
  }
  
  return bnd;
}

void PolTreeState::new_leaf(const IntVarArray& vars, const IntVar& util)
{
  // in leaf, compute upward
  assert(util.assigned());
  int vars_size = vars.size();
  
  //* first, get evidence and get probability
  // current evidence; pair<int,int> = (varBNid, value)
  int index=0;
  for (int i=0; i!=vars_size; i++) {
    int bn_id = varBNid[i];
    if (bn_id != -1) // AND node
      leaf_evidence[index++].second = vars[i].val();
  }
  // get marginal probability of current state
  double prob = bn.pr(leaf_evidence);
  
  //* then, compute value over all OR nodes and also all AND nodes if assigned to last value
  double child_val = prob*util.val(); // for OR nodes
  double child_diff = child_val; // for AND nodes
  int parent=vars_size-1;
  if (verbose >= 2)
    cout << "In leaf! exputil="<<child_val<<" with prob "<<prob<<endl;
  if (verbose >= 8) {
    cout << "Util by CP: " << util.val() << " util by manual IA: " << this->max_f_vars(vars) << endl;
    assert(util.val() == this->max_f_vars(vars));
  }
  for (; parent >= 0; parent--) {
    int bn_id = varBNid[parent];
    if (bn_id == -1) { // OR node (max)
      if (child_val <= evals[parent])
        break; // no improvement upwards
      if (evals[parent] == min_inf)
        child_diff = child_val; // ORs are inited -inf, ANDs as 0
      else
        child_diff = child_val - evals[parent];
      if (verbose >= 2)
        cout << "updating OR["<<parent<<"] from "<<evals[parent]<<" to "<<child_val<<" (diff="<<child_diff<<")\n";
      evals[parent] = child_val;
        
    } else { // AND node (sum)
      // update 'evals' values up to a parent OR node
      int x = parent;
      while (x >= 0 && varBNid[x] != -1) {
        if (verbose >= 2)
          cout << "updating AND["<<x<<"] from "<<evals[x];
        evals[x] += child_diff;
        child_val = evals[x];
        if (verbose >= 2)
          cout <<" to "<<evals[x]<<" (diff="<<child_diff<<")\n";
        x--;
      }
      
      //* AND node state maintance (up to parent OR node)
      bool did_break = false;
      while (parent >= 0 && varBNid[parent] != -1) {
        // reset own value
        brch_rand_val[parent] = -1;
        // only continue upwards if all children of the AND node are visited
        if (!brch_rand_lastval[parent]) {
          did_break = true;
          break;
        }
        
        else { // is last val, reset the previous random variable's val (certifies non-failed AND child)
          int prev=parent-1;
          while (prev >= 0 && varBNid[prev] == -1)
            prev--;
          if (prev >= 0) { // found previous AND var
            if (verbose >= 2)
              cout << "Marking prev AND var: "<<prev<<" with bn_id "<<varBNid[prev]<<" as fully explored\n";
            brch_rand_val[prev] = -1;
          }
        }
        parent--;
      }
      parent += 1; // because one parent-- too much above
      if (did_break)
        break;
    }
    
  }
  if (parent < 0) {
    // got to parent, update exputil var
    cout << "*** New root: " << vars[0] << " exputil: " << evals[0] << "\n";
  } 
}

double PolTreeState::max_f_vars(const IntVarArray& vars)
{
  // init the input for the utility
  for (int i=0; i!=vars.size(); i++) {
    varsmima[i].first = vars[i].min();
    varsmima[i].second = vars[i].max();
  }
  return max_f(varsmima);
}
