// forge:v1
#include "eval.h"

#include <array>
#include <initializer_list>

#include <stdio.h>

#include "log.h"

namespace taocp::eval {

// === --- Literal --------------------------------------------------------- ===
//

auto
Literal::newPos( ssize_t v ) -> Literal
{
        if ( v <= 0 ) PANIC( "var id must be positive, got %zd", v );
        return Literal{ v };
}

auto
Literal::newNeg( ssize_t v ) -> Literal
{
        if ( v <= 0 ) PANIC( "var id must be positive, got %zd", v );
        return Literal{ -v };
}

auto
Literal::fromDimacs( ssize_t l ) -> Literal
{
        if ( l > 0 ) return newPos( l );
        if ( l == 0 ) PANIC( "DIMACS literal must be nonzero" );
        return newNeg( -l );
}

// === --- Bench ----------------------------------------------------------- ===
//

namespace {

/// Builds a `Bench` from clauses written in DIMACS notation: each literal is a
/// nonzero integer, `k` for x_k and `-k` for ¬x_k (see `Literal::fromDimacs`).
auto
from_clauses( BenchInfo                                             info,
              std::initializer_list<std::initializer_list<ssize_t>> cs )
    -> Bench
{
        Bench b{ info, { }, { } };
        b.segments.reserve( cs.size( ) );
        for ( auto &c : cs ) {
                size_t begin = b.clauses.size( );
                for ( ssize_t l : c )
                        b.clauses.push_back( Literal::fromDimacs( l ) );
                b.segments.push_back( { begin, b.clauses.size( ) } );
        }
        return b;
}

/// waerden(3,3;n): for every arithmetic progression i, i+d, i+2d in 1..=n, the
/// clauses (x_i x_{i+d} x_{i+2d}) and (¬x_i ¬x_{i+d} ¬x_{i+2d}). All positive
/// clauses come first, then all negative ones.
auto
waerden_3_3( BenchInfo info ) -> Bench
{
        auto n = (ssize_t)info.num_vars;

        // Stage 1: generate all arithmetic progression (ap).
        //
        std::vector<std::array<ssize_t, 3>> aps;
        for ( ssize_t d = 1; d <= ( n - 1 ) / 2; d++ ) {
                for ( ssize_t i = 1; i <= n - 2 * d; i++ ) {
                        aps.push_back( { i, i + d, i + 2 * d } );
                }
        }

        // Stage 2: fill into clauses and segments
        // - 2 is due to poss and negs.
        // - 3 is due to the number of literals in each ap
        //
        Bench b{ info, { }, { } };
        b.clauses.reserve( 2 * 3 * aps.size( ) );
        b.segments.reserve( 2 * aps.size( ) );
        for ( auto new_lit : { Literal::newPos, Literal::newNeg } ) {
                for ( auto &ap : aps ) {
                        size_t begin = b.clauses.size( );
                        for ( ssize_t v : ap )
                                b.clauses.push_back( new_lit( v ) );
                        b.segments.push_back( { begin, b.clauses.size( ) } );
                }
        }
        return b;
}

}  // namespace

auto
Bench::get( ID id ) -> Bench
{
        switch ( id ) {
        case ID::Rivest:
                return from_clauses( BenchInfo{
                                         /*vol=*/"4b",
                                         /*page=*/"188",
                                         /*eqn=*/"6",
                                         /*name=*/"rivest",
                                         /*num_vars=*/4,
                },
                                     {
                                         { 1, 2, -3 },
                                         { 2, 3, -4 },
                                         { 3, 4, 1 },
                                         { 4, -1, 2 },
                                         { -1, -2, 3 },
                                         { -2, -3, 4 },
                                         { -3, -4, -1 },
                                         { -4, 1, -2 },
                                     } );
        case ID::Waerden339:
                return waerden_3_3( BenchInfo{
                    /*vol=*/"4b",
                    /*page=*/"188",
                    /*eqn=*/"9",
                    /*name=*/"waerden(3,3;9)",
                    /*num_vars=*/9,
                } );
        }
        PANIC( "unknown bench id %d", (int)id );
        return { };
}

auto
printBenchInfo( const BenchInfo &info ) -> void
{
        printf( "BenchInfo {\n" );
        printf( "    vol: \"%s\",\n", info.vol );
        printf( "    page: \"%s\",\n", info.page );
        printf( "    eqn: \"%s\",\n", info.eqn );
        printf( "    name: \"%s\",\n", info.name );
        printf( "    num_vars: %zu,\n", info.num_vars );
        printf( "}\n" );
}

}  // namespace taocp::eval
