#pragma once
#pragma GCC diagnostic ignored "-Wswitch"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"

#include <vector>
#include <exception>

#include <gecode/float.hh>
#include <gecode/search.hh>
#include <gecode/int.hh>

#include "cm_options.h"
#include "policy_tree_state.h"

using std::vector;
using std::ostream;
using std::cerr;
using std::endl;

using Gecode::IntVar;
using Gecode::IntVarArray;

class Inv2Model : public Gecode::Space {
 public:
  IntVarArray vars;
  IntVar util; // XXX we don't actually need it... could use f_max on leaf

 public:
  // constructor, creates the model 
  Inv2Model(PolTreeState&, Cm_opt&);
  
  // constructor, copying
  Inv2Model(bool share, Inv2Model& s) :
  Gecode::Space(share, s), vars(s.vars) {
    vars.update(*this, share, s.vars);
    util.update(*this, share, s.util);
  }
  
  // copy function, calls copy constructor
  virtual Gecode::Space* copy(bool share) {
    return new Inv2Model(share, *this);
  }
  
  // printing
  virtual void print(ostream& os) const {
    try{
      os << "New leaf: " << vars << " util: " << util << "\n";
      os << "--------\n"; 
    } catch (std::exception e) {
      cerr << "ERROR \n" << e.what() << endl;
    }
  }
  
};

