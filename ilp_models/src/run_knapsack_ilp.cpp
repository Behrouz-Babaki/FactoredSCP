#include <iostream>
#include <vector>
#include <cstdlib>
#include <ace_engine_cpp.hpp>

#include "cm_options.h"
#include "knapsack_ilp.h"

using std::vector;
using std::cout;
using std::endl;
using std::cerr;

int main(int argc, char** argv) {

  getCmOptions(argc, argv);

    if (PROG_OPT.capacity < 0) {
        cerr << "capacity must be a non-negative integer" << endl;
        exit(1);
    }
    
    try {
      
        AceEngineCpp engine(PROG_OPT.ac_file, PROG_OPT.lm_file, 1, PROG_OPT.verbose);
        GRBEnv* env = new GRBEnv();
	env->set(GRB_IntParam_UpdateMode, 1);
	env->set(GRB_IntParam_Threads, 1);
        KnapsackModelILP* b = new KnapsackModelILP(env, engine, PROG_OPT);
        delete b;
        delete env;
    }
    catch (GRBException e)
    {
        cout << "Error code = " << e.getErrorCode() << endl;
        cout << e.getMessage() << endl;
    }
    catch (...)
    {
        cout << "Exception during optimization" << endl;
    }


    return 0;
}
