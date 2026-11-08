// forge:skip
//
// main — prints the info of a SAT benchmark problem.
//
// Usage:
//   main
//
// Takes no arguments. Builds the waerden(3,3;9) benchmark (TAOCP Vol 4B, Page
// 188, Eq 9) and prints its BenchInfo to stdout.
#include "eval.h"

using taocp::eval::Bench;
using taocp::eval::ID;

int
main( )
{
    auto bench = Bench::get( ID::Waerden339 );
    taocp::eval::printBenchInfo( &bench.info );
    return 0;
}
