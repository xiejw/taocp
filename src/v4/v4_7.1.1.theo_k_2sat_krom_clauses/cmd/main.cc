// See two_sat_solver.h
//
// forge:skip
#include "test_macros.h"
#include "two_sat_solver.h"

namespace {
using namespace taocp;

FORGE_TEST( test_single_node_sat )
{
        // a || C(a)          C(a) -> C(a)
        TwoSatSolver s{ 1 };
        const size_t a = 0;
        s.addKromClause( a, s.getComplementId( a ) );
        bool sat = s.checkSatisfiability( );

        EXPECT_TRUE( sat, "true" );

        return NULL;
}

FORGE_TEST( test_single_node_contradict )
{
        // a || a               C(a) -> a
        // C(a) || C(a)         a -> C(a)
        TwoSatSolver s{ 1 };
        const size_t a = 0;
        s.addKromClause( a, a );
        s.addKromClause( s.getComplementId( a ), s.getComplementId( a ) );
        bool sat = s.checkSatisfiability( );

        EXPECT_TRUE( !sat, "not sat" );

        return NULL;
}

FORGE_TEST( test_two_nodes )
{
        // C(a) || C(b)          a -> C(b)
        //   b  || C(a)          C(b) -> C(a)
        TwoSatSolver s{ 2 };
        const size_t a = 0;
        const size_t b = 1;
        s.addKromClause( s.getComplementId( a ), s.getComplementId( b ) );
        s.addKromClause( b, s.getComplementId( a ) );
        bool sat = s.checkSatisfiability( );

        EXPECT_TRUE( sat, "true" );

        return NULL;
}

// Problem (37) on Vol 4A, Page 60. See also (39) on Page 61.
FORGE_TEST( test_comedians )
{
        TwoSatSolver s{ 7 };
        const size_t t = 0;
        const size_t u = 1;
        const size_t v = 2;
        const size_t w = 3;
        const size_t x = 4;
        const size_t y = 5;
        const size_t z = 6;

        s.addKromClause( s.getComplementId( t ), s.getComplementId( w ) );
        s.addKromClause( s.getComplementId( u ), s.getComplementId( z ) );
        s.addKromClause( u, s.getComplementId( y ) );
        s.addKromClause( u, z );
        s.addKromClause( s.getComplementId( y ), z );
        s.addKromClause( t, s.getComplementId( x ) );
        s.addKromClause( t, z );
        s.addKromClause( s.getComplementId( x ), z );
        s.addKromClause( s.getComplementId( t ), s.getComplementId( z ) );
        s.addKromClause( s.getComplementId( v ), y );
        s.addKromClause( v, s.getComplementId( w ) );
        s.addKromClause( v, s.getComplementId( y ) );
        s.addKromClause( s.getComplementId( w ), s.getComplementId( y ) );
        s.addKromClause( u, x );
        s.addKromClause( s.getComplementId( u ), v );
        s.addKromClause( s.getComplementId( v ), s.getComplementId( x ) );

        bool sat = s.checkSatisfiability( );

        EXPECT_TRUE( !sat, "not sat" );

        return NULL;
}

}  // namespace

int
main( )
{
        forge::test_suite_run( );
}
