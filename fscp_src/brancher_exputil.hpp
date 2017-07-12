// brancher for use with constraint_exputil and bn_engine.
// based on MPG 'minsize' brancher
#ifndef _BRANCHER_EXPUTIL_HPP_
#define _BRANCHER_EXPUTIL_HPP_

#include <gecode/int.hh>

#include "policy_tree_state.h"

using namespace Gecode;

// sorting on second pair entry
template <typename T1, typename T2>
struct greater_second {
  typedef std::pair<T1, T2> type;
  bool operator ()(type const& a, type const& b) const {
    return a.second > b.second;
  }
};

void branch_exputil(Home home, 
                    const IntVarArgs& x, 
                    PolTreeState& poltree, 
                    bool order_or_max, 
                    bool order_and_max, 
                    int depth_or, 
                    int depth_and);

class BranchExpUtil : public Brancher {
protected:
  // variables
  Gecode::ViewArray<Int::IntView> vars;
  
  // state
  PolTreeState& poltree;
  mutable int pos;
  const bool order_or_max; // for OR nodes, max value first or min value first
  const bool order_and_max;
  const int depth_or;
  const int depth_and;
  
  class VarChoice : public Choice {
  public:
    int id;
    std::vector<int> vals;
    std::vector<double> upperbounds;
    VarChoice(const BranchExpUtil& b, int id0, const std::vector<int>& vals0, const std::vector<double>& ubs)
      : Choice(b, vals0.size()), id(id0), vals(vals0), upperbounds(ubs) {}
    virtual size_t size(void) const {
      return sizeof(*this);
    }
    virtual void archive(Archive& e) const {
      Choice::archive(e);
      e << id;
      e << (int)vals.size();
      for(size_t i = 0; i < vals.size(); i++) {
        e<<vals[i];
        e<<upperbounds[i];
      }
    }
  };
public:
  
  // for rand vars, we branch based on mass
  // for decision vars... min value (could be an option)
  BranchExpUtil(Home home, ViewArray<Int::IntView>& vars0, PolTreeState& poltree0,
                bool order_or_max0, bool order_and_max0, int depth_or0, int depth_and0)
    : Brancher(home), vars(vars0), poltree(poltree0), pos(0),
      order_or_max(order_or_max0), order_and_max(order_and_max0), depth_or(depth_or0), depth_and(depth_and0) {}
    
  static void post(Home home, ViewArray<Int::IntView>& vars, PolTreeState& poltree, 
                   bool order_or_max, bool order_and_max, int depth_or, int depth_and) {
    (void) new (home) BranchExpUtil(home,vars,poltree,order_or_max,order_and_max,depth_or,depth_and);
  }
  
  virtual size_t dispose(Space& home) {
    (void) Brancher::dispose(home);
    return sizeof(*this);
  }
  
  BranchExpUtil(Space& home, bool share, BranchExpUtil& b)
    : Brancher(home,share,b), poltree(b.poltree), pos(b.pos), 
      order_or_max(b.order_or_max), order_and_max(b.order_and_max), depth_or(b.depth_or), depth_and(b.depth_and) {
    vars.update(home,share,b.vars);
  }
  
  virtual Brancher* copy(Space& home, bool share) {
    return new (home) BranchExpUtil(home,share,*this);
  }
  
  // check whether there is something to branch on
  virtual bool status(const Space& home) const {
    for (int i=pos; i<vars.size(); i++) {
      if (!vars[i].assigned()) {
        pos = i; return true;
      }
    }
    return false;
  }
  
  // return one Choice object that contains how many values to branch over
  // this is called just once per variable
  virtual Choice* choice(Space& home);
   // something technical, depends on Choice* implementation
  virtual Choice* choice(const Space&, Archive& e) {
    int id;
    int n;
    e >> id >> n;
    
    std::vector<int> vals;
    vals.reserve(n);
    int v;
    std::vector<double> ubs;
    ubs.reserve(n);
    int ub;
    
    for(int i=0; i < n; i++) {
      e >> v;
      vals.push_back(v);
      e >> ub;
      ubs.push_back(ub);
    }
    
    return new VarChoice(*this, id, vals, ubs);
  }
  
  // c is our Choice implementation
  // a is the how-manyth value of this choice
  // this is called for each value of the variable
  virtual ExecStatus commit(Space& home, 
                            const Choice& c,
                            unsigned int a) ;
			    
  // printing Choice nr a to o.
  virtual void print(const Space& home, const Choice& c,
                     unsigned int a,
                     std::ostream& o) const {
    const VarChoice& vc = static_cast<const VarChoice&>(c);
    int id = vc.id;
    int val = vc.vals[a];
    o << "x[" << id << "] = " << val;
  }
  
  void reset_frompos(int pos);
};

#endif
