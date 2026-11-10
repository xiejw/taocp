// forge:skip
#include <string.h>

#include <vector>

#include "../../../sat/src/eval.h"
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
        printClause( lits.size( ), lits.data( ) );
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

    INFO( "Emit Clauses:" );
    emitBench( &solver, &bench );

    if ( auto res = solver.searchOneSolution( ); res ) {
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
    runSolver( eval::ID::Rivest );
    return 0;
}
