// forge:skip
//
// main — prints the info of every SAT benchmark problem.
//
// Usage:
//   main
//
// Takes no arguments. Builds each benchmark returned by `getAllIDs()` and
// prints its BenchInfo to stdout. Set `TAOCP_SAT_EVAL_ID` to print only the
// benchmark with that 0-based index (see `getAllIDs()` in `sat/eval.h`).
#include "sat/eval.h"

using taocp::sat::eval::Bench;
using taocp::sat::eval::getAllIDs;
using taocp::sat::eval::printBenchInfo;

int
main( )
{
    for ( auto id : getAllIDs( ) ) {
        auto bench = Bench::get( id );
        printBenchInfo( &bench );
    }
    return 0;
}
