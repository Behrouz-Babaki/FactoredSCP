#pragma GCC diagnostic ignored "-Wunused-local-typedefs"

#include <iostream>
#include <cstdlib>
#include <sys/resource.h>

#include <gecode/float.hh>
#include <gecode/int.hh>
#include <gecode/search.hh>
#include <ace_engine_cpp.hpp>

#include "inv2_model.h"
#include "cm_options.h"
#include "policy_tree_state.h"

using std::cout;
using std::cerr;
using std::endl;

// TODO explicit scope declaration
using namespace Gecode;


// for timing
// from: http://stackoverflow.com/questions/17432502/how-can-i-measure-cpu-time-and-wall-clock-time-on-both-linux-windows
#include <time.h>
#include <sys/time.h>
double get_wall_time(){
  struct timeval time;
  if (gettimeofday(&time,NULL)){
    //  Handle error
    return 0;
  }
  return (double)time.tv_sec + (double)time.tv_usec * .000001;
}
double get_cpu_time(){
  return (double)clock() / CLOCKS_PER_SEC;
}

// struct to store command-line options
extern struct Cm_opt PROG_OPT;

int main(int argc, char** argv) {

    // parse command-line options
    getCmOptions(argc, argv);

    try {

	int verbose = PROG_OPT.verbose;
        // init BNEngine(AC,LM,cache_level,verbosity)
        AceEngineCpp engine_c(PROG_OPT.ac_file, PROG_OPT.lm_file, 1, verbose);
        PolTreeState poltree(engine_c, verbose);

        // Create the problem
        Inv2Model* s = new Inv2Model(poltree, PROG_OPT);
	
	if (PROG_OPT.verbose >= 0) {
	  cout << "ac file: " << PROG_OPT.ac_file << endl;
	  cout << "lmap file: " << PROG_OPT.lm_file << endl;
	  cout << "depth_and: " << PROG_OPT.depth_and << endl;
	  cout << "depth_or: " << PROG_OPT.depth_or << endl;
	}

        Search::Options o;
        o.c_d = 0; // recomputation must ABSOLUTELY be disabled!
        o.a_d = 0; // recomputation must ABSOLUTELY be disabled!


        int sols = 0;
        double start = get_wall_time();

        DFS<Inv2Model> d(s,o);
        while (Inv2Model* current_solution = d.next()) {
            sols++;
            if (verbose >= 1)
                current_solution->print(cout);
            poltree.new_leaf(current_solution->vars, current_solution->util);
            delete current_solution;
        }

        Search::Statistics stats = d.statistics();
        if (true) { // (opt.verbose() >= 0) {
            cout << "Solver stats:"
                 << " time: " << get_wall_time() - start
                 << " sols: " << sols
                 << " fails: " << stats.fail
                 << " nodes: " << stats.node
                 << " props: " << stats.propagate
                 << endl;
        }

        delete s;

    } catch (Exception e) {
        cerr << "Something went wrong ...\n" << e.what() << endl;
    }

    return 0;
}
