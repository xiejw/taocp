// forge:skip
#include <string.h>

#include <vector>

#include "sat/eval.h"
#include "sat/timer.h"

#include "log.h"
#include "sat_literal.h"
#include "sat_watch.h"

using taocp::complement;
using taocp::Literal;
using taocp::printClause;
using taocp::WatchSolver;

namespace eval = taocp::sat::eval;

namespace {

/// Translates the eval literal `lit` (x_k or ¬x_k) into the solver's encoding.
auto
toSolverLiteral( eval::Literal lit ) -> Literal
{
    auto [var, pos] = lit.getVarAndSign( );
    return pos ? var : complement( var );
}

/// Emits every clause of `bench` into `solver`, printing each one.
auto
emitBench( WatchSolver *solver, const eval::Bench *bench ) -> void
{
    std::vector<Literal> lits;
    for ( auto &seg : bench->segments ) {
        lits.clear( );
        for ( size_t i = seg.begin; i < seg.end; i++ ) {
            lits.push_back( toSolverLiteral( bench->clauses[i] ) );
        }
        // printClause( lits.size( ), lits.data( ) );
        solver->emitClause( lits.size( ), lits.data( ) );
    }
}

auto
runSolver( eval::ID id ) -> void
{
    INFO( "==== --- Algorithm B: Satisfiability by Watching --- ===" );
    auto bench = eval::Bench::get( id );
    eval::printBenchInfo( &bench.info );

    WatchSolver solver{ /*num_vars=*/bench.info.num_vars,
                        /*num_clauses=*/bench.segments.size( ) };

    taocp::sat::Timer timer;

    INFO( "Emit Clauses:" );
    timer.start( );
    emitBench( &solver, &bench );
    timer.stopAndReport( "emitBench" );

    timer.start( );
    auto res = solver.searchOneSolution( );
    timer.stopAndReport( "searchOneSolution" );

    if ( res ) {
        INFO( "Satisfiable!!!" );
        INFO( "Result:" );
        printClause( res.value( ).size( ), res.value( ).data( ) );
    } else {
        INFO( "Unsatisfiable" );
    }
}
}  // namespace

int
main( )
{
    for ( auto id : eval::getAllIDs( ) ) runSolver( id );
    return 0;
}
