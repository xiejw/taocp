// forge:skip
#include "eval.h"

#include "test_macros.h"

#include <stdint.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

using taocp::eval::Bench;
using taocp::eval::ID;
using taocp::eval::Literal;

namespace {

/// Brute-force: true if some assignment satisfies every clause.
///
/// Tries all 2^n assignments, where n = `b->info.num_vars`. An assignment is a
/// uint64_t `bits` whose bit k-1 holds the value of x_k (1 = true), so counting
/// `bits` from 0 to 2^n - 1 visits every assignment exactly once.
///
/// For each assignment, every clause (one `{begin, end}` segment) must be true;
/// a clause is true if any of its literals is true. Literal x_k is true when
/// bit k-1 is 1, and ¬x_k when that bit is 0. The loops stop early, so the
/// search ends at the first satisfying assignment.
///
/// Cost is O(2^n * total literals), which is fine for the small benchmarks
/// here; it requires n <= 63 so `1 << n` fits in a uint64_t.
auto
satisfiable_by_brute_force( const Bench *b ) -> bool
{
    for ( uint64_t bits = 0; bits < ( uint64_t{ 1 } << b->info.num_vars );
          bits++ ) {
        bool all = true;
        for ( auto &seg : b->segments ) {
            bool any = false;
            for ( size_t j = seg.begin; j < seg.end; j++ ) {
                ssize_t l = b->clauses[j].v;
                auto    k = (uint64_t)( l > 0 ? l : -l );
                bool    v = ( ( bits >> ( k - 1 ) ) & 1 ) == 1;
                if ( l > 0 ? v : !v ) {
                    any = true;
                    break;
                }
            }
            if ( !any ) {
                all = false;
                break;
            }
        }
        if ( all ) return true;
    }
    return false;
}

/// True if `fn` terminates the process abnormally (e.g. via PANIC). Runs `fn`
/// in a forked child so the test process survives.
auto
dies( void ( *fn )( ) ) -> bool
{
    fflush( stdout );
    pid_t pid = fork( );
    if ( pid == 0 ) {
        // Silence the panic message in the child.
        freopen( "/dev/null", "w", stderr );
        freopen( "/dev/null", "w", stdout );
        fn( );
        _exit( 0 );
    }
    int status = 0;
    waitpid( pid, &status, 0 );
    return !( WIFEXITED( status ) && WEXITSTATUS( status ) == 0 );
}

}  // namespace

FORGE_TEST( get_returns_num_vars )
{
    EXPECT_TRUE( Bench::get( ID::Rivest ).info.num_vars == 4, "rivest" );
    EXPECT_TRUE( Bench::get( ID::Waerden339 ).info.num_vars == 9, "waerden" );
    return NULL;
}

FORGE_TEST( literal_new )
{
    EXPECT_TRUE( Literal::newPos( 3 ) == Literal{ 3 }, "pos" );
    EXPECT_TRUE( Literal::newNeg( 3 ) == Literal{ -3 }, "neg" );
    return NULL;
}

FORGE_TEST( literal_from_dimacs )
{
    EXPECT_TRUE( Literal::fromDimacs( 3 ) == Literal::newPos( 3 ), "pos" );
    EXPECT_TRUE( Literal::fromDimacs( -3 ) == Literal::newNeg( 3 ), "neg" );
    return NULL;
}

FORGE_TEST( literal_from_dimacs_rejects_zero )
{
    EXPECT_TRUE( dies( [] { Literal::fromDimacs( 0 ); } ), "should panic" );
    return NULL;
}

FORGE_TEST( literal_new_pos_rejects_zero )
{
    EXPECT_TRUE( dies( [] { Literal::newPos( 0 ); } ), "should panic" );
    return NULL;
}

FORGE_TEST( literal_new_neg_rejects_negative )
{
    EXPECT_TRUE( dies( [] { Literal::newNeg( -2 ); } ), "should panic" );
    return NULL;
}

FORGE_TEST( clause_counts )
{
    auto r = Bench::get( ID::Rivest );
    EXPECT_TRUE( r.segments.size( ) == 8, "rivest segments" );
    EXPECT_TRUE( r.clauses.size( ) == 24, "rivest clauses" );
    auto w = Bench::get( ID::Waerden339 );
    EXPECT_TRUE( w.segments.size( ) == 32, "waerden segments" );
    EXPECT_TRUE( w.clauses.size( ) == 96, "waerden clauses" );
    return NULL;
}

FORGE_TEST( segments_tile_clauses )
{
    for ( auto id : { ID::Rivest, ID::Waerden339 } ) {
        auto   b   = Bench::get( id );
        size_t pos = 0;
        for ( auto &seg : b.segments ) {
            EXPECT_TRUE( seg.begin == pos, "contiguous" );
            EXPECT_TRUE( seg.end > seg.begin, "non-empty" );
            pos = seg.end;
        }
        EXPECT_TRUE( pos == b.clauses.size( ), "covers all" );
    }
    return NULL;
}

FORGE_TEST( all_unsatisfiable )
{
    auto rivest  = Bench::get( ID::Rivest );
    auto waerden = Bench::get( ID::Waerden339 );
    EXPECT_TRUE( !satisfiable_by_brute_force( &rivest ), "rivest" );
    EXPECT_TRUE( !satisfiable_by_brute_force( &waerden ), "waerden" );
    return NULL;
}

FORGE_TEST( rivest_minus_last_clause_is_satisfiable )
{
    auto r = Bench::get( ID::Rivest );
    r.segments.pop_back( );
    EXPECT_TRUE( satisfiable_by_brute_force( &r ), "satisfiable" );
    return NULL;
}

int
main( )
{
    forge::test_suite_run( );
    return 0;
}
