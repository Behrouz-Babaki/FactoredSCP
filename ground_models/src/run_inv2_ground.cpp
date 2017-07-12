#pragma GCC diagnostic ignored "-Wunused-local-typedefs"

#include <iostream>
#include <gecode/search.hh>
#include <cstdlib>
#include <sys/resource.h>
#include <ace_engine_cpp.hpp>
#ifdef USE_GIST
#include <gecode/gist.hh>
#endif //USE_GIST


#include "cm_options.h"
#include "inv2_ground.h"
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
        Inv2GModel* m = new Inv2GModel(engine, PROG_OPT);

        double start = get_wall_time();
        int sols = 0;
        int timeout = PROG_OPT.timeout;

        #ifdef USE_GIST
        Gecode::Gist::Print<KnapsackGModel> p("Print solution");
        Gecode::Gist::Options o;
        o.c_d = 0; // disable recomputation
        o.inspect.click(&p);
        Gecode::Gist::bab(m, o);
        #else
        Options o;
        o.c_d = 0; // disable recomputation
        #endif //USE_GIST
        if (timeout > 0)
            o.stop = new TimeStop(timeout*1e3);

        BAB<Inv2GModel> e(m, o);
        double best_value = -1;
        while (Inv2GModel* s = e.next()) {
            sols++;
            //s->print();
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
