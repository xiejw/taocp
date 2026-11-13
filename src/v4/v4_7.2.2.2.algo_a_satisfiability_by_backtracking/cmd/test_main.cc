// forge:skip
#include "sat_backtrack.h"
#include "sat_literal.h"
#include "test_macros.h"

using taocp::BacktrackSolver;
using taocp::complement;
using taocp::Literal;
using taocp::literal;
using taocp::Segment;

namespace {

/// All clauses of a test problem, laid out like `eval::Bench`.
struct Table {
    std::vector<Literal> clauses;
    std::vector<Segment> segments;
};

/// Builds a `Table` from `cs`, one inner list per clause.
auto
makeTable( const std::initializer_list<std::initializer_list<Literal>> *cs )
    -> Table
{
    Table t;
    for ( auto &c : *cs ) {
        size_t begin = t.clauses.size( );
        for ( auto lit : c ) t.clauses.push_back( lit );
        t.segments.push_back( { begin, t.clauses.size( ) } );
    }
    return t;
}

}  // namespace

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
    // clang-format off
    std::initializer_list<std::initializer_list<Literal>> cs{
        /* === --- Consistency ------------------------------------------ ===
         */
        { complement( literal( 1 ) ) , complement( literal( 2 ) )  },
        { complement( literal( 1 ) ) , complement( literal( 3 ) )  },
        { complement( literal( 2 ) ) , complement( literal( 3 ) )  },

        { complement( literal( 4 ) ) , complement( literal( 5 ) )  },
        { complement( literal( 4 ) ) , complement( literal( 6 ) )  },
        { complement( literal( 5 ) ) , complement( literal( 6 ) )  },

        { complement( literal( 7 ) ) , complement( literal( 8 ) )  },
        { complement( literal( 7 ) ) , complement( literal( 9 ) )  },
        { complement( literal( 7 ) ) , complement( literal( 10 ) )  },
        { complement( literal( 7 ) ) , complement( literal( 11 ) )  },
        { complement( literal( 8 ) ) , complement( literal( 9 ) )  },
        { complement( literal( 8 ) ) , complement( literal( 10 ) )  },
        { complement( literal( 8 ) ) , complement( literal( 11 ) )  },
        { complement( literal( 9 ) ) , complement( literal( 10 ) )  },
        { complement( literal( 9 ) ) , complement( literal( 11 ) )  },
        { complement( literal( 10 ) ) , complement( literal( 11 ) )  },

        { complement( literal( 12 ) ) , complement( literal( 13 ) )  },
        { complement( literal( 12 ) ) , complement( literal( 14 ) )  },
        { complement( literal( 12 ) ) , complement( literal( 15 ) )  },
        { complement( literal( 12 ) ) , complement( literal( 16 ) )  },
        { complement( literal( 13 ) ) , complement( literal( 14 ) )  },
        { complement( literal( 13 ) ) , complement( literal( 15 ) )  },
        { complement( literal( 13 ) ) , complement( literal( 16 ) )  },
        { complement( literal( 14 ) ) , complement( literal( 15 ) )  },
        { complement( literal( 14 ) ) , complement( literal( 16 ) )  },
        { complement( literal( 15 ) ) , complement( literal( 16 ) )  },

        /* === --- Dependencies ----------------------------------------- ===
        */
        { complement( literal( 2 ) ), literal( 5 )  },
        { complement( literal( 2 ) ), literal( 13 ), literal( 14 ), literal( 15 ), literal( 16 )  },

        { complement( literal( 3 ) ), literal( 5 ), literal( 6 )  },
        { complement( literal( 3 ) ), literal( 14 ), literal( 15 ), literal( 16 )  },

        { complement( literal( 4 ) ), literal( 14 ), literal( 15 ), literal( 16 )  },
        { complement( literal( 5 ) ), literal( 14 ), literal( 15 ), literal( 16 )  },
        { complement( literal( 6 ) ), literal( 15 ), literal( 16 )  },

        { complement( literal( 7 ) ), literal( 12 ), literal( 13 ), literal( 14 ), literal( 15 )  },
        { complement( literal( 8 ) ), literal( 12 ), literal( 13 ), literal( 14 ), literal( 15 )  },
        { complement( literal( 9 ) ), literal( 13 ), literal( 14 ), literal( 15 ), literal( 16 )  },
        { complement( literal( 10 ) ), literal( 14 ), literal( 15 ), literal( 16 )  },
        { complement( literal( 11 ) ), literal( 15 ), literal( 16 )  },

        /* === --- Root  ------------------------------------------------ ===
        */
        { literal( 3 )  },
        { literal( 6 )  },
        { literal( 10 ), literal( 11 )  },
        { literal( 16 )  },
    };
    // clang-format on
    auto            table = makeTable( &cs );
    BacktrackSolver solver{ /*num_vars=*/16, &table.clauses, &table.segments };

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
            EXPECT_TRUE( res.value( )[i] == literal( i + 1 ), "[i] == i+1" );
            break;
        default:
            EXPECT_TRUE( res.value( )[i] == complement( literal( i + 1 ) ),
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
    std::initializer_list<std::initializer_list<Literal>> cs{
        { literal( 1 ) },
        { literal( 1 ), complement( literal( 2 ) ) },
        { literal( 2 ), complement( literal( 3 ) ) },
        { literal( 2 ), literal( 3 ) },
    };
    auto            table = makeTable( &cs );
    BacktrackSolver solver{ /*num_vars=*/3, &table.clauses, &table.segments };

    auto res = solver.searchOneSolution( );

    EXPECT_TRUE( bool( res ) == true, "has answer" );
    EXPECT_TRUE( res.value( ).size( ) == 3, "3 eles" );
    EXPECT_TRUE( res.value( )[0] == literal( 1 ), "[0] == 1" );
    EXPECT_TRUE( res.value( )[1] == literal( 2 ), "[1] == 2" );
    EXPECT_TRUE( res.value( )[2] == complement( literal( 3 ) ), "[2] == c3" );
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
    std::initializer_list<std::initializer_list<Literal>> cs{
        { literal( 3 ) },
        { complement( literal( 2 ) ), literal( 1 ) },
        { literal( 3 ), literal( 2 ) },
        { complement( literal( 3 ) ), complement( literal( 1 ) ) },
        { literal( 3 ), complement( literal( 1 ) ) },
    };
    auto            table = makeTable( &cs );
    BacktrackSolver solver{ /*num_vars=*/3, &table.clauses, &table.segments };

    auto res = solver.searchOneSolution( );

    EXPECT_TRUE( bool( res ) == true, "has answer" );
    EXPECT_TRUE( res.value( ).size( ) == 3, "3 eles" );
    EXPECT_TRUE( res.value( )[0] == complement( literal( 1 ) ), "[0] == c1" );
    EXPECT_TRUE( res.value( )[1] == complement( literal( 2 ) ), "[1] == c2" );
    EXPECT_TRUE( res.value( )[2] == literal( 3 ), "[2] == 3" );
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
    std::initializer_list<std::initializer_list<Literal>> cs{
        { complement( literal( 1 ) ) },
        { literal( 1 ), complement( literal( 2 ) ) },
        { literal( 2 ), complement( literal( 3 ) ) },
        { literal( 2 ), literal( 3 ) },
    };
    auto            table = makeTable( &cs );
    BacktrackSolver solver{ /*num_vars=*/3, &table.clauses, &table.segments };

    auto res = solver.searchOneSolution( );

    EXPECT_TRUE( bool( res ) == false, "no answer" );
}

int
main( )
{
    ::forge::test_suite_run( );
    return 0;
}
