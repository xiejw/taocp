// forge:skip
#include "sat_literal.h"
#include "sat_watch.h"
#include "test_macros.h"

using taocp::C;
using taocp::WatchSolver;

FORGE_TEST( dpe_resolution )
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
        WatchSolver sov{ /*num_literals=*/16,
                         /*num_causes=*/3 * 2 + 10 * 2 + 16 };

        // clang-format off
        /* === --- Consistency ------------------------------------------ ===
         */
        sov.emitClause(  { C( 1 ) , C( 2 )  } );
        sov.emitClause(  { C( 1 ) , C( 3 )  } );
        sov.emitClause(  { C( 2 ) , C( 3 )  } );

        sov.emitClause(  { C( 4 ) , C( 5 )  } );
        sov.emitClause(  { C( 4 ) , C( 6 )  } );
        sov.emitClause(  { C( 5 ) , C( 6 )  } );

        sov.emitClause(  { C( 7 ) , C( 8 )  } );
        sov.emitClause(  { C( 7 ) , C( 9 )  } );
        sov.emitClause(  { C( 7 ) , C( 10 )  } );
        sov.emitClause(  { C( 7 ) , C( 11 )  } );
        sov.emitClause(  { C( 8 ) , C( 9 )  } );
        sov.emitClause(  { C( 8 ) , C( 10 )  } );
        sov.emitClause(  { C( 8 ) , C( 11 )  } );
        sov.emitClause(  { C( 9 ) , C( 10 )  } );
        sov.emitClause(  { C( 9 ) , C( 11 )  } );
        sov.emitClause(  { C( 10 ) , C( 11 )  } );

        sov.emitClause(  { C( 12 ) , C( 13 )  } );
        sov.emitClause(  { C( 12 ) , C( 14 )  } );
        sov.emitClause(  { C( 12 ) , C( 15 )  } );
        sov.emitClause(  { C( 12 ) , C( 16 )  } );
        sov.emitClause(  { C( 13 ) , C( 14 )  } );
        sov.emitClause(  { C( 13 ) , C( 15 )  } );
        sov.emitClause(  { C( 13 ) , C( 16 )  } );
        sov.emitClause(  { C( 14 ) , C( 15 )  } );
        sov.emitClause(  { C( 14 ) , C( 16 )  } );
        sov.emitClause(  { C( 15 ) , C( 16 )  } );

        /* === --- Dependencies ----------------------------------------- ===
        */
        sov.emitClause(  { C( 2 ), 5  } );
        sov.emitClause(  { C( 2 ), 13, 14, 15, 16  } );

        sov.emitClause(  { C( 3 ), 5, 6  } );
        sov.emitClause(  { C( 3 ), 14, 15, 16  } );

        sov.emitClause(  { C( 4 ), 14, 15, 16  } );
        sov.emitClause(  { C( 5 ), 14, 15, 16  } );
        sov.emitClause(  { C( 6 ), 15, 16  } );

        sov.emitClause(  { C( 7 ), 12, 13, 14, 15  } );
        sov.emitClause(  { C( 8 ), 12, 13, 14, 15  } );
        sov.emitClause(  { C( 9 ), 13, 14, 15, 16  } );
        sov.emitClause(  { C( 10 ), 14, 15, 16  } );
        sov.emitClause(  { C( 11 ), 15, 16  } );

        /* === --- Root  ------------------------------------------------ ===
        */
        sov.emitClause(  { 3  } );
        sov.emitClause(  { 6  } );
        sov.emitClause(  { 10, 11  } );
        sov.emitClause(  { 16  } );

        // clang-format on
        auto res = sov.searchOneSolution( );

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
                        EXPECT_TRUE( res.value( )[i] == C( i + 1 ),
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
        WatchSolver sov{ /*num_literals=*/3, /*num_causes=*/4 };

        sov.emitClause( { ( 1 ) } );
        sov.emitClause( { 1, C( 2 ) } );
        sov.emitClause( { 2, C( 3 ) } );
        sov.emitClause( { 2, 3 } );

        auto res = sov.searchOneSolution( );

        EXPECT_TRUE( bool( res ) == true, "has answer" );
        EXPECT_TRUE( res.value( ).size( ) == 3, "3 eles" );
        EXPECT_TRUE( res.value( )[0] == 1, "[0] == 1" );
        EXPECT_TRUE( res.value( )[1] == 2, "[1] == 2" );
        EXPECT_TRUE( res.value( )[2] == C( 3 ), "[2] == c3" );
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
        WatchSolver sov{ /*num_literals=*/3, /*num_causes=*/5 };

        sov.emitClause( { 3 } );
        sov.emitClause( { C( 2 ), 1 } );
        sov.emitClause( { 3, 2 } );
        sov.emitClause( { C( 3 ), C( 1 ) } );
        sov.emitClause( { 3, C( 1 ) } );

        auto res = sov.searchOneSolution( );

        EXPECT_TRUE( bool( res ) == true, "has answer" );
        EXPECT_TRUE( res.value( ).size( ) == 3, "3 eles" );
        EXPECT_TRUE( res.value( )[0] == C( 1 ), "[0] == c1" );
        EXPECT_TRUE( res.value( )[1] == C( 2 ), "[1] == c2" );
        EXPECT_TRUE( res.value( )[2] == 3, "[2] == 3" );
}

FORGE_TEST( no_result )
{
        /* clauses
         * c1
         * 1 c2
         * 2 c3
         * 2 3
         *
         * answer is: no solution.
         */
        WatchSolver sov{ /*num_literals=*/3, /*num_causes=*/4 };

        sov.emitClause( { C( 1 ) } );
        sov.emitClause( { 1, C( 2 ) } );
        sov.emitClause( { 2, C( 3 ) } );
        sov.emitClause( { 2, 3 } );

        auto res = sov.searchOneSolution( );

        EXPECT_TRUE( bool( res ) == false, "no answer" );
}

int
main( )
{
        ::forge::test_suite_run( );
        return 0;
}
