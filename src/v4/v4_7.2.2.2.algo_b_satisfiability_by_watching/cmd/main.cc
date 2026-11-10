// forge:skip
#include <string.h>

#include "log.h"
#include "sat_literal.h"
#include "sat_watch.h"

using taocp::complement;
using taocp::Literal;
using taocp::printClause;
using taocp::WatchSolver;

namespace {
auto
printAndEmitClause( WatchSolver *solver, std::initializer_list<Literal> lits )
{
    printClause( lits.size( ), std::data( lits ) );
    solver->emitClause( lits );
}

auto
runSolver( ) -> void
{
    INFO( "==== --- Demo Problem for Algorithm B --- ===" );
    INFO( "==== --- Satisfiability by Watching   --- ===" );
    WatchSolver solver{ /*num_vars=*/3, /*num_clauses=*/4 };

    INFO( "Emit Clauses:" );
    printAndEmitClause( &solver, { 1 } );
    printAndEmitClause( &solver, { 1, complement( 2 ) } );
    printAndEmitClause( &solver, { 2, complement( 3 ) } );
    printAndEmitClause( &solver, { 2, 3 } );

    // INFO( "Debug Print:" );
    // solver.dumpDebugInfo( );

    if ( auto res = solver.searchOneSolution( ); res ) {
        INFO( "Satisfiable!!!" );
        INFO( "Result:" );
        printClause( res.value( ).size( ), res.value( ).data( ) );
    } else {
        PANIC( "Unsatisfiable" );
    }
}
}  // namespace

int
main( )
{
    runSolver( );
    return 0;
}
