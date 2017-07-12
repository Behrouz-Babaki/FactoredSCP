#include <iostream>
#include <vector>
#include <cstdlib>
#include <ace_engine_cpp.hpp>

#include "cm_options.h"
#include "finance_ilp.h"

using std::vector;
using std::cout;
using std::endl;
using std::cerr;

int main(int argc, char** argv) {

  getCmOptions(argc, argv);
  if (PROG_OPT.data_file == NULL) {
    cerr << "data file is missing" << endl;
    exit(1);
  }

    try {
      
      AceEngineCpp engine(PROG_OPT.ac_file, PROG_OPT.lm_file, 1, PROG_OPT.verbose);
        GRBEnv* env = new GRBEnv();
        FinanceModelILP* b = new FinanceModelILP(env, engine, PROG_OPT);
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
