// brancher for use with constraint_exputil and bn_engine.
// based on MPG 'minsize' brancher

#include <gecode/int.hh>

#include "brancher_exputil.hpp"

using namespace Gecode;
using namespace std;

void branch_exputil(Home home, const IntVarArgs& x, PolTreeState& poltree,
                    bool order_or_max, bool order_and_max, int depth_or, int depth_and) {
  if (home.failed()) return;
  ViewArray<Int::IntView> y(home,x);
  BranchExpUtil::post(home,y,poltree,order_or_max,order_and_max,depth_or,depth_and);
}

inline
void BranchExpUtil::reset_frompos(int pos)
{
  const vector<int>& varBNid = poltree.varBNid;
  int size = varBNid.size();
  
  // reset evals of this and future nodes in policy tree (up to next AND)
  // because assigned ORs (if only one choice) are otherwise skipped
  for (int i=pos; i!=size; i++) {
    bool is_and = (varBNid[i] != -1);
    if (!is_and) { // OR node
      poltree.evals[i] = poltree.min_inf;
      if (i == 0)
        poltree.lb[i] = poltree.min_inf;
      else
        poltree.lb[i] = poltree.lb[i-1];
    } else { // AND node
      poltree.evals[i] = 0;
      poltree.lb[i] = poltree.min_inf;
      if (i != pos)
        break; // optimisation: up to next AND
    }
    //cout << "Choice init of evals["<<i<<"] (is_and:"<<is_and<<") = "<< poltree.evals[i] <<"\n";
  }
}

 
// return one Choice object that contains how many values to branch over
// this is called just once per variable, just after calling 'status()', which sets 'pos'
// or variables: returns all choices, from smallest val to highest
// and variables; returns all non-zero probability choices, from most to least probable
Choice* BranchExpUtil::choice(Space& home)
{
    reset_frompos(pos);
    
    const vector<int>& varBNid = poltree.varBNid;
    bool is_and = (varBNid[pos] != -1);
    
    // determine value order
    vector< std::pair<int,double> > score_val;
    if (!is_and) { // OR node, (will sort decreasing so positive if order_or_max, negative otherwise)
      /* 
       * Behrouz[xpl]: The entire domain of an integer variable can be accessed by a value iterator IntVarValues. 
       * The call operator i() tests whether there are more values to iterate for i , the prefix increment
       * operator ++i moves the iterator i to the next value, and i.val() returns the current value
       * of the iterator i 
       */
      for (IntVarValues i(vars[pos]); i(); ++i) {
        int v = i.val();
        if (!order_or_max)
          v = -v;
        score_val.push_back( std::make_pair(i.val(), v) );
      }
      
    } else { // AND node, order based on expected utility bound, largest first
      if (depth_and < 0) { // special case: do not check for 0-prob vals
        for (IntVarValues i(vars[pos]); i(); ++i) {
          int v = i.val();
          if (!order_and_max)
            v = -v;
          score_val.push_back( std::make_pair(i.val(), v) );
        }
      } else {
        // get probabilities
        vector< pair<int,int> > evidence; // varBNid,val
        for (int i=0; i!=pos; i++) {
          if (varBNid[i] != -1) // earlier (assigned) AND node
            evidence.push_back( std::make_pair(varBNid[i], vars[i].val()) );
        }
        score_val = poltree.bn.var_partials(evidence, varBNid[pos]);
        
        // remove 0 probabilities
        size_t lst = score_val.size();
        for (size_t i = lst; i--;) {
          if (score_val[i].second == 0)
            std::swap(score_val[i], score_val[--lst]);
        }
        score_val.resize(lst);
        
        // replace probability by upper bounds on exputil
        if (depth_and >= 0 && pos != 0 && poltree.lb[pos-1] != poltree.min_inf)
          poltree.bounds_and(vars, pos, score_val, depth_and);
      }
    }
    
    // sort by score and put in vector
    std::sort(score_val.begin(), score_val.end(), greater_second<int, double>());
    vector<int> val_order;
    vector<double> ubs;
    for (auto it=score_val.begin(); it!=score_val.end(); ++it) {
      val_order.push_back(it->first);
      if (is_and)
        ubs.push_back(it->second);
    }
    
    if (poltree.verbose >= 5)
      std::cout << "Saving "<<val_order.size()<<" choices for position "<<pos<<std::endl;
    return new VarChoice(*this, pos, val_order, ubs);
}
  
  // c is our Choice implementation
  // a is the how-manyth value of this choice
  // this is called for each value of the variable
ExecStatus BranchExpUtil::commit(Space& home, const Choice& c, unsigned int a)
{
    const VarChoice& vc = static_cast<const VarChoice&>(c);
    int pos = vc.id;
    int bn_id = poltree.varBNid[pos];
    bool is_and = (bn_id != -1);
    int val = vc.vals[a];
    
    // AND variable with non-first value, did the previous child succeed?
    if (is_and) {
      if (a != 0 && poltree.brch_rand_val[pos] != -1) {
        // our previous branch choice did not reach a leaf for all children
        if (poltree.verbose >= 2)
          std::cout << "in brancher, brch_rand_val not -1 for rand var "<<pos<<", fail!\n";
        return ES_FAILED;
      }
      
      // AND variable, store new choice of this child
      poltree.brch_rand_val[pos] = val;
      if (a == vc.vals.size()-1)
        poltree.brch_rand_lastval[pos] = true;
      else
        poltree.brch_rand_lastval[pos] = false;
      // and reset stuff below (e.g. ORs that may already be assigned)
      reset_frompos(pos+1);
    }    
    
    // try branching
    if (me_failed(vars[vc.id].eq(home, val)))
      return ES_FAILED;
    
    // compute bound!
    if (is_and) { // AND node
      if (pos != 0 && poltree.lb[pos-1] != poltree.min_inf) { // not as first var, needs parent, and only if enabled
        // so many months later, I forgot why for AND the lb is pos-1? (no pruning for lb[pos]...)
        if (depth_and == 0) { // simplest bound
          double bnd = poltree.bound_or(vars, pos, 0); // at depth 0, same for OR and AND
          if (poltree.verbose >= 2)
            cout << "Bound for AND node (depth=0)"<<pos<<"\twith val: " << vars[pos].val() << " and lb " << poltree.lb[pos-1] << " :: " << bnd << "\n";
        
          if (!(bnd > poltree.lb[pos-1])) {
            // can prune
            if (poltree.verbose >= 2)
              cout << "Pruning based on bound\n";
            return ES_FAILED;
          }
        }
        if (depth_and > 0) { // proper bound
          // lb = LB_node - \sum_seen val_seen - \sum_unseen UB_unseen
          double sum_ub_unseen = 0;
          for (size_t x=a+1; x!=vc.upperbounds.size(); x++)
            sum_ub_unseen += vc.upperbounds[x];
          double lb = poltree.lb[pos-1] - poltree.evals[pos] - sum_ub_unseen;
          // check that computed UB is > lb:
          if (!(vc.upperbounds[a] > lb)) {
            if (poltree.verbose >= 2)
              cout << "And node pos="<<pos<<" with value "<<val<<": PRUNED (lb="<<lb<<" > "<<vc.upperbounds[a]<<")\n";
            return ES_FAILED;
          }
          poltree.lb[pos] = lb;
        }
      }
      
    } else { // OR node
      if (poltree.evals[pos] != poltree.min_inf) // previous child's val
        poltree.lb[pos] = poltree.evals[pos];
      if (poltree.lb[pos] != poltree.min_inf && depth_or >= 0) { // OR node with a bound and depth_or >= 0
        
        double bnd = poltree.bound_or(vars, pos, depth_or);
        if (poltree.verbose >= 2)
          cout << "Bound for OR node "<<pos<<"\twith val: " << vars[pos].val() << " and lb " << poltree.lb[pos] << " :: " << bnd << "\n";
        
        if (!(bnd > poltree.lb[pos])) {
          // can prune
          if (poltree.verbose >= 2)
            cout << "Pruning based on bound\n";
          return ES_FAILED;
        }
      }
    }
    
    return ES_OK;
}

