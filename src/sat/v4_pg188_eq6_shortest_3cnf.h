// vim: ft=cpp
#pragma once

#define C( x ) x

class SATSolver;

namespace taocp {
int
emitShortest3CNF( SATSolver *solver, std::string *err_msg )
{
        solver->emitSimpleClause( { 1, 2, C( 3 ) } );
        solver->emitSimpleClause( { 2, 3, C( 4 ) } );
        solver->emitSimpleClause( { 3, 4, 1 } );
        solver->emitSimpleClause( { 4, C( 1 ), 2 } );
        solver->emitSimpleClause( { C( 1 ), C( 2 ), 3 } );
        solver->emitSimpleClause( { C( 2 ), C( 3 ), 4 } );
        solver->emitSimpleClause( { C( 3 ), C( 4 ), C( 1 ) } );
        solver->emitSimpleClause( { C( 4 ), 1, C( 2 ) } );
}
}  // namespace taocp
