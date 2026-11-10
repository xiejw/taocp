// forge:skip
#include "sat_literal.h"
#include "sat_watch.h"
#include "test_macros.h"

using taocp::complement;
using taocp::WatchSolver;

FORGE_TEST( depResolution )
{
    /* See source
     * https://borretti.me/article/dependency-resolution-made-simple
     *
     * Assignments (* is final selection)
     *  1 sql:0
     *  2 sql:1
     *  3 sql:2*
     *
     *  4 threads:0
     *  5 threads:1
     *  6 threads:2*
     *
     *  7 http:0
     *  8 http:1
     *  9 http:2
     * 10 http:3
     * 11 http:4*
     *
     * 12 stdlib:0
     * 13 stdlib:1
     * 14 stdlib:2
     * 15 stdlib:3
     * 16 stdlib:4*
     */
    WatchSolver solver{ /*num_vars=*/16,
                        /*num_clauses=*/3 * 2 + 10 * 2 + 16 };

    // clang-format off
        /* === --- Consistency ------------------------------------------ ===
         */
        solver.emitClause(  { complement( 1 ) , complement( 2 )  } );
        solver.emitClause(  { complement( 1 ) , complement( 3 )  } );
        solver.emitClause(  { complement( 2 ) , complement( 3 )  } );

        solver.emitClause(  { complement( 4 ) , complement( 5 )  } );
        solver.emitClause(  { complement( 4 ) , complement( 6 )  } );
        solver.emitClause(  { complement( 5 ) , complement( 6 )  } );

        solver.emitClause(  { complement( 7 ) , complement( 8 )  } );
        solver.emitClause(  { complement( 7 ) , complement( 9 )  } );
        solver.emitClause(  { complement( 7 ) , complement( 10 )  } );
        solver.emitClause(  { complement( 7 ) , complement( 11 )  } );
        solver.emitClause(  { complement( 8 ) , complement( 9 )  } );
        solver.emitClause(  { complement( 8 ) , complement( 10 )  } );
        solver.emitClause(  { complement( 8 ) , complement( 11 )  } );
        solver.emitClause(  { complement( 9 ) , complement( 10 )  } );
        solver.emitClause(  { complement( 9 ) , complement( 11 )  } );
        solver.emitClause(  { complement( 10 ) , complement( 11 )  } );

        solver.emitClause(  { complement( 12 ) , complement( 13 )  } );
        solver.emitClause(  { complement( 12 ) , complement( 14 )  } );
        solver.emitClause(  { complement( 12 ) , complement( 15 )  } );
        solver.emitClause(  { complement( 12 ) , complement( 16 )  } );
        solver.emitClause(  { complement( 13 ) , complement( 14 )  } );
        solver.emitClause(  { complement( 13 ) , complement( 15 )  } );
        solver.emitClause(  { complement( 13 ) , complement( 16 )  } );
        solver.emitClause(  { complement( 14 ) , complement( 15 )  } );
        solver.emitClause(  { complement( 14 ) , complement( 16 )  } );
        solver.emitClause(  { complement( 15 ) , complement( 16 )  } );

        /* === --- Dependencies ----------------------------------------- ===
        */
        solver.emitClause(  { complement( 2 ), 5  } );
        solver.emitClause(  { complement( 2 ), 13, 14, 15, 16  } );

        solver.emitClause(  { complement( 3 ), 5, 6  } );
        solver.emitClause(  { complement( 3 ), 14, 15, 16  } );

        solver.emitClause(  { complement( 4 ), 14, 15, 16  } );
        solver.emitClause(  { complement( 5 ), 14, 15, 16  } );
        solver.emitClause(  { complement( 6 ), 15, 16  } );

        solver.emitClause(  { complement( 7 ), 12, 13, 14, 15  } );
        solver.emitClause(  { complement( 8 ), 12, 13, 14, 15  } );
        solver.emitClause(  { complement( 9 ), 13, 14, 15, 16  } );
        solver.emitClause(  { complement( 10 ), 14, 15, 16  } );
        solver.emitClause(  { complement( 11 ), 15, 16  } );

        /* === --- Root  ------------------------------------------------ ===
        */
        solver.emitClause(  { 3  } );
        solver.emitClause(  { 6  } );
        solver.emitClause(  { 10, 11  } );
        solver.emitClause(  { 16  } );

    // clang-format on
    auto res = solver.searchOneSolution( );

    EXPECT_TRUE( bool( res ) == true, "has answer" );
    EXPECT_TRUE( res.value( ).size( ) == 16, "16 eles" );
    for ( size_t i = 0; i < res.value( ).size( ); i++ ) {
        switch ( i + 1 ) {
        case 3:
            // passthrough
        case 6:
            // passthrough
        case 11:
            // passthrough
        case 16:
            EXPECT_TRUE( res.value( )[i] == i + 1, "[i] == i+1" );
            break;
        default:
            EXPECT_TRUE( res.value( )[i] == complement( i + 1 ),
                         "[i] == C(i+1)" );
        }
    }
}

FORGE_TEST( simple )
{
    /* clauses
     * 1
     * 1 c2
     * 2 c3
     * 2 3
     *
     * answer is 1 2 any 3
     */
    WatchSolver solver{ /*num_vars=*/3, /*num_clauses=*/4 };

    solver.emitClause( { ( 1 ) } );
    solver.emitClause( { 1, complement( 2 ) } );
    solver.emitClause( { 2, complement( 3 ) } );
    solver.emitClause( { 2, 3 } );

    auto res = solver.searchOneSolution( );

    EXPECT_TRUE( bool( res ) == true, "has answer" );
    EXPECT_TRUE( res.value( ).size( ) == 3, "3 eles" );
    EXPECT_TRUE( res.value( )[0] == 1, "[0] == 1" );
    EXPECT_TRUE( res.value( )[1] == 2, "[1] == 2" );
    EXPECT_TRUE( res.value( )[2] == complement( 3 ), "[2] == c3" );
}

FORGE_TEST( reverse )
{
    /* clauses
     * 3
     * c2 1
     * 3 2
     * c3 c1
     * 3 c1
     *
     * answer is c1 c2 3
     */
    WatchSolver solver{ /*num_vars=*/3, /*num_clauses=*/5 };

    solver.emitClause( { 3 } );
    solver.emitClause( { complement( 2 ), 1 } );
    solver.emitClause( { 3, 2 } );
    solver.emitClause( { complement( 3 ), complement( 1 ) } );
    solver.emitClause( { 3, complement( 1 ) } );

    auto res = solver.searchOneSolution( );

    EXPECT_TRUE( bool( res ) == true, "has answer" );
    EXPECT_TRUE( res.value( ).size( ) == 3, "3 eles" );
    EXPECT_TRUE( res.value( )[0] == complement( 1 ), "[0] == c1" );
    EXPECT_TRUE( res.value( )[1] == complement( 2 ), "[1] == c2" );
    EXPECT_TRUE( res.value( )[2] == 3, "[2] == 3" );
}

FORGE_TEST( noResult )
{
    /* clauses
     * c1
     * 1 c2
     * 2 c3
     * 2 3
     *
     * answer is: no solution.
     */
    WatchSolver solver{ /*num_vars=*/3, /*num_clauses=*/4 };

    solver.emitClause( { complement( 1 ) } );
    solver.emitClause( { 1, complement( 2 ) } );
    solver.emitClause( { 2, complement( 3 ) } );
    solver.emitClause( { 2, 3 } );

    auto res = solver.searchOneSolution( );

    EXPECT_TRUE( bool( res ) == false, "no answer" );
}

int
main( )
{
    ::forge::test_suite_run( );
    return 0;
}
