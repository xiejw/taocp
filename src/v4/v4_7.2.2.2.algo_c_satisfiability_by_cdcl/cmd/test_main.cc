// forge:skip
#include "test_macros.h"

#include "sat_cdcl.h"
#include "sat_literal.h"

using taocp::C;
using taocp::CDCLSolver;
using taocp::decodeRawLiteralValue;
using taocp::isLiteralComplement;

FORGE_TEST( Complement )
{
        auto c = C( 1 );
        EXPECT_TRUE( decodeRawLiteralValue( c ) == 1, "decode" );
        EXPECT_TRUE( isLiteralComplement( c ) == true, "isC" );
}

FORGE_TEST( Simple )
{
        /* Clauses
         * 1
         * 1 c2
         * 2 c3
         * 2 3
         *
         * answer is 1 2 any 3
         */
        CDCLSolver sov{ };

        sov.emitClause( { ( 1 ) } );
        sov.emitClause( { 1, C( 2 ) } );
        sov.emitClause( { 2, C( 3 ) } );
        sov.emitClause( { 2, 3 } );

        auto res = sov.searchOneSolution( );

        // EXPECT_TRUE( bool( res ) == true, "has answer" );
        // EXPECT_TRUE( res.value( ).size( ) == 3, "3 eles" );
        // EXPECT_TRUE( res.value( )[0] == 1, "[0] == 1" );
        // EXPECT_TRUE( res.value( )[1] == 2, "[1] == 2" );
        // EXPECT_TRUE( res.value( )[2] == C( 3 ), "[2] == c3" );
}

int
main( )
{
        ::forge::test_suite_run( );
        return 0;
}
