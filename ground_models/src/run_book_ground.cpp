#pragma GCC diagnostic ignored "-Wunused-local-typedefs"

#include <iostream>
#include <sys/resource.h>
#include <gecode/search.hh>
#include <ace_engine_cpp.hpp>

#include "cm_options.h"
#include "book_ground.h"
#include "time_util.hpp"

using std::cin;
using std::cout;
using std::endl;
using std::cerr;

using Gecode::BAB;
using Gecode::Exception;
using Gecode::Search::Statistics;
using Gecode::Search::Options;
using Gecode::Search::TimeStop;

int main(int argc, char* argv[]) {

    extern struct Cm_opt PROG_OPT;
    getCmOptions(argc, argv);

    try {

        AceEngineCpp engine(PROG_OPT.ac_file, PROG_OPT.lm_file, 1, PROG_OPT.verbose);
        // Create the problem
        BookGModel* m = new BookGModel(engine, PROG_OPT);
	int sols = 0;
	double best_value = -1;
        int timeout = PROG_OPT.timeout;

	Options o;
        if (timeout > 0)
            o.stop = new TimeStop(timeout*1e3);
	
        double start = get_wall_time();
        BAB<BookGModel> e(m, o);
        while (BookGModel* s = e.next()) {
            //s->print();
	    sols++;
	    best_value = s->solution_value();
            delete s;
        }
        
        if (timeout > 0) {
            if (e.stopped())
                cout << "search stopped" << endl;
            else
                cout << "solution found" << endl;
        }
        
        cout << "exputil: " << best_value << endl;
	Statistics stats = e.statistics();
        if (true) { // (opt.verbose() >= 0) {
            cout << "Solver stats:"
                 << " time: " << get_wall_time() - start
                 << " sols: " << sols
                 << " fails: " << stats.fail
                 << " nodes: " << stats.node
                 << " props: " << stats.propagate
                 << endl;
        }
        delete m;

    } catch (Exception e) {
        cerr << "Something went wrong ...\n" << e.what() << endl;
    }

    return 0;
}
