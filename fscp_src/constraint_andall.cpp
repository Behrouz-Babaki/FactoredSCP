//
// seq_embedding.cpp
//
// global embedding relation for sequence mining
// 
// Made by Tias Guns <tias.guns@cs.kuleuven.be>

#include <set>
#include <utility>

#include "constraint_andall.hpp"

using namespace Gecode; 
using namespace Int;
using namespace Float;
using namespace std; 


// constraint post function
void and_nodes_all(Gecode::Space& home,
                   const Gecode::IntVarArgs& vars,
                   const PolTreeState& poltree
) {
  if (home.failed()) return;
  const vector<int>& varBNid = poltree.varBNid;
  
  IntVarArgs and_nodes;
  IntArgs and_idx;
  IntArgs and_size;
  for (int i=0; i!=vars.size(); i++) {
    if (varBNid[i] != -1) { // AND node
      and_nodes << vars[i];
      and_idx << i;
      and_size << poltree.bn.domain_size(varBNid[i]);
    }
  }
  
  ViewArray<IntView> and_vars(home, and_nodes);
  IntSharedArray and_idx0(and_idx);
  IntSharedArray and_size0(and_size);
  GECODE_ES_FAIL((ConsAndAll<IntView>::post(home, and_vars, and_idx0, and_size0, poltree)));
}


template <class VA>
ExecStatus ConsAndAll<VA>::propagate(Space& home, const ModEventDelta& )
{
  //TODO: would make for a perfect advisor propagator... (each var independently)
  if (home.failed())
    return home.ES_SUBSUMED(*this);
  
  int vars_size = and_vars.size();
  
  // for all assigned AND nodes: check that the brancher chose this value (if not: some world is impossible)
  while (pos != vars_size && and_vars[pos].assigned()) {
    int idx = and_idx[pos];
    if (poltree.brch_rand_val[idx] != and_vars[pos].val()) {
      if (poltree.verbose >= 2)
        cout << "and_var["<<pos<<"] idx="<<idx<<" is not the same as given by brancher: "<<poltree.brch_rand_val[idx]<<"\n";
      return ES_FAILED;
    }
    pos++;
  }
  
  // for all remaining AND nodes, check that full domain
  // if not, a possible world is impossible and we can already backtrack...
  for (int x=pos; x!=vars_size; x++) {
    int idx = and_idx[x];
    if (and_vars[x].size() != (size_t)and_size[x]) {
      if (poltree.verbose >= 2)
        cout << "NO MATCH var["<<x<<"] idx="<<idx<<", " << and_vars[x] << " size: "<<and_vars[x].size()<<" vs domsize " << and_size[x] << "\n";
      return ES_FAILED;
    }
  }
  
  return ES_FIX;
}

