//
// constraint to compute expected utility
// 
// Made by Tias Guns <tias.guns@cs.kuleuven.be>

#ifndef _CONS_ANDALL_HPP_
#define _CONS_ANDALL_HPP_

#include <gecode/int.hh>
#include <gecode/float.hh>

#include "policy_tree_state.h"
#include <map>

using std::map;

// post constraint
void and_nodes_all(Gecode::Space& home,
                   const Gecode::IntVarArgs& vars, // array with decision and random variables
                   const PolTreeState& poltree // the policy tree state
                  );

// propagator definition
template <class VA>
class ConsAndAll : public Gecode::Propagator {
protected:
    // variables
    Gecode::ViewArray<VA> and_vars;
    Gecode::IntSharedArray and_idx;
    Gecode::IntSharedArray and_size;
    
    int pos; // first unassigned and_var (initially 0)
    const PolTreeState& poltree;

public:
    // posting
    static Gecode::ExecStatus post(Gecode::Space& home,
                                   Gecode::ViewArray<VA>& and_vars,
                                   Gecode::IntSharedArray& and_idx,
                                   Gecode::IntSharedArray& and_size,
                                   const PolTreeState& poltree
                                  ) {
      (void) new (home) ConsAndAll<VA>(home,and_vars,and_idx,and_size,poltree);
      return Gecode::ES_OK;
    }
    
    // post constructor
    ConsAndAll(Gecode::Space& home,
               Gecode::ViewArray<VA>& and_vars0,
               Gecode::IntSharedArray& and_idx0,
               Gecode::IntSharedArray& and_size0,
               const PolTreeState& poltree0
               )
    : Propagator(home), and_vars(and_vars0), and_idx(and_idx0), and_size(and_size0), pos(0), poltree(poltree0)
    {
      and_vars.subscribe(home,*this,Gecode::Int::PC_INT_DOM);
    }
    
    // copy constructor
    ConsAndAll(Gecode::Space& home, bool share, ConsAndAll& p)
    : Propagator(home,share,p), and_idx(p.and_idx), and_size(p.and_size), pos(p.pos), poltree(p.poltree)
    {
      and_vars.update(home,share,p.and_vars);
    }
    
    virtual size_t dispose(Gecode::Space& home)
    {
      and_vars.cancel(home,*this,Gecode::Int::PC_INT_VAL);
      (void) Propagator::dispose(home);
      return sizeof(*this);
    }

    virtual Gecode::Propagator* copy(Gecode::Space& home, bool share)
    {
      return new (home) ConsAndAll<VA>(home,share,*this);
    }

    virtual Gecode::PropCost cost(const Gecode::Space&, const Gecode::ModEventDelta&) const
    {
      // run as last
      return Gecode::PropCost::linear(Gecode::PropCost::LO, and_vars.size());
    }
    
  
    // propagation
    virtual Gecode::ExecStatus propagate(Gecode::Space& home, const Gecode::ModEventDelta&);    
    
};

#endif
