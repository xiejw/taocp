// vim: ft=cpp
// forge:skip
//
// Benchmark problems for evaluating SAT solvers. Header-only.
//
// Environment variables:
//
//   TAOCP_SAT_EVAL_ID  If set, `getAllIDs()` returns only the benchmark at
//                      this 0-based index in `enum class ID` (declaration
//                      order), instead of all of them. For example,
//                      `TAOCP_SAT_EVAL_ID=2 ./.build/main` runs only
//                      waerden(3,3;9). Panics if the value is not an integer
//                      in [0, kNumIDs).
#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <array>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <vector>

namespace taocp::sat::eval {

// === --- Panic ----------------------------------------------------------- ===
//

namespace detail {

/// Prints `fmt` (printf-style) with the source location to stderr, then
/// aborts the process.
[[noreturn]] inline auto
panic( const char *file, int line, const char *fmt, ... ) -> void
{
    ::fprintf( stderr, "PANIC [%s:%d] ", file, line );
    va_list args;
    va_start( args, fmt );
    ::vfprintf( stderr, fmt, args );
    va_end( args );
    ::fprintf( stderr, "\n" );
    ::abort( );
}

}  // namespace detail

#define TAOCP_EVAL_PANIC( ... ) \
    ::taocp::sat::eval::detail::panic( __FILE__, __LINE__, __VA_ARGS__ )

// === --- Literal --------------------------------------------------------- ===
//

/// A literal: either x_k or ¬x_k, with k >= 1. The encoding is private; build
/// one with `newPos`, `newNeg` or `fromDimacs` and inspect it with
/// `getVarAndSign`.
///
/// The encoding favors readability over efficiency: it exists only to describe
/// benchmark problems. Real solver algorithms should convert to a better
/// encoding (e.g., TAOCP's 2k / 2k+1 scheme for x_k / ¬x_k) instead of using
/// this one directly.
class Literal {
  public:
    /// Returns the positive literal x_v. Panics unless `v` is positive.
    static auto newPos( ssize_t v ) -> Literal
    {
        if ( v <= 0 ) TAOCP_EVAL_PANIC( "var id must be positive, got %zd", v );
        return Literal{ v };
    }

    /// Returns the negative literal ¬x_v. Panics unless `v` is positive.
    static auto newNeg( ssize_t v ) -> Literal
    {
        if ( v <= 0 ) TAOCP_EVAL_PANIC( "var id must be positive, got %zd", v );
        return Literal{ -v };
    }

    /// Parses DIMACS notation: `k` means x_k and `-k` means ¬x_k. Panics if
    /// `l` is zero.
    static auto fromDimacs( ssize_t l ) -> Literal
    {
        if ( l > 0 ) return newPos( l );
        if ( l == 0 ) TAOCP_EVAL_PANIC( "DIMACS literal must be nonzero" );
        return newNeg( -l );
    }

    /// Returns `{k, pos}`: the variable id k >= 1 and whether the literal is
    /// positive (`pos` is true for x_k and false for ¬x_k).
    auto getVarAndSign( ) const -> std::pair<size_t, bool>
    {
        if ( v > 0 ) return { static_cast<size_t>( v ), true };
        return { static_cast<size_t>( -v ), false };
    }

    auto operator==( Literal o ) const -> bool { return v == o.v; }

  private:
    /// `k` for x_k and `-k` for ¬x_k.
    ssize_t v;

    explicit Literal( ssize_t v ) : v{ v } {}
};

// === --- Bench and ID ---------------------------------------------------- ===
//

/// Identifies a benchmark problem. Add new IDs before `Count`, which is a
/// sentinel holding the number of IDs and is not a valid benchmark.
enum class ID {
    Rivest,
    Rivest7,
    Waerden339,
    Langford3,
    Langford9,
    Count,
};

/// The number of benchmark problems.
inline constexpr size_t kNumIDs = static_cast<size_t>( ID::Count );

/// Returns every benchmark ID, in declaration order.
///
/// If the environment variable `TAOCP_SAT_EVAL_ID` is set, returns only the ID
/// whose index (0-based, in declaration order) it names, e.g.
/// `TAOCP_SAT_EVAL_ID=2` for `ID::Waerden339`. Panics if the value is not an
/// integer in `[0, kNumIDs)`.
inline auto
getAllIDs( ) -> std::vector<ID>
{
    if ( const char *env = ::getenv( "TAOCP_SAT_EVAL_ID" ); env != nullptr ) {
        char *end = nullptr;
        long  idx = ::strtol( env, &end, 10 );
        if ( *env == '\0' || *end != '\0' || idx < 0 ||
             static_cast<size_t>( idx ) >= kNumIDs ) {
            TAOCP_EVAL_PANIC(
                "TAOCP_SAT_EVAL_ID must be in [0, %zu), got \"%s\"", kNumIDs,
                env );
        }
        return { static_cast<ID>( idx ) };
    }

    std::vector<ID> ids;
    ids.reserve( kNumIDs );
    for ( size_t i = 0; i < kNumIDs; i++ )
        ids.push_back( static_cast<ID>( i ) );
    return ids;
}

/// Where a benchmark problem comes from in TAOCP.
struct BenchInfo {
    const char *vol;
    const char *page;
    const char *eqn;
    const char *name;
    const char *desc;  /// A compact description of the problem.
    size_t      num_vars;
    /// Whether the clauses are satisfiable (known from TAOCP).
    bool satisfiable;
};

/// Clause j is `clauses[begin, end)` where `{begin, end} = segments[j]`.
struct Segment {
    size_t begin;
    size_t end;
};

/// Asserts at compile time that `T` has the same layout as `eval::Segment`
/// (trivially copyable, same size, `begin` and `end` at the same offsets), so
/// `eval::Segment`s can be copied into `T`s with one memcpy. Use at namespace
/// scope, e.g. `TAOCP_EVAL_ASSERT_SEGMENT_LAYOUT( taocp::Segment );`.
#define TAOCP_EVAL_ASSERT_SEGMENT_LAYOUT( T )                              \
    static_assert( std::is_trivially_copyable_v<T> );                      \
    static_assert(                                                         \
        std::is_trivially_copyable_v<::taocp::sat::eval::Segment> );       \
    static_assert( sizeof( T ) == sizeof( ::taocp::sat::eval::Segment ) ); \
    static_assert( offsetof( T, begin ) ==                                 \
                   offsetof( ::taocp::sat::eval::Segment, begin ) );       \
    static_assert( offsetof( T, end ) ==                                   \
                   offsetof( ::taocp::sat::eval::Segment, end ) )

/// A benchmark problem.
struct Bench {
    BenchInfo info;
    /// All literals of all clauses, laid out one clause after another.
    std::vector<Literal> clauses;
    std::vector<Segment> segments;

    /// Returns the benchmark problem identified by `id`.
    static auto get( ID id ) -> Bench;
};

namespace detail {

/// Builds a `Bench` from clauses written in DIMACS notation: each literal is a
/// nonzero integer, `k` for x_k and `-k` for ¬x_k (see `Literal::fromDimacs`).
inline auto
fromClauses( const BenchInfo                                             *info,
             const std::initializer_list<std::initializer_list<ssize_t>> *cs )
    -> Bench
{
    Bench b{ *info, { }, { } };
    b.segments.reserve( cs->size( ) );
    for ( auto &c : *cs ) {
        size_t begin = b.clauses.size( );
        for ( ssize_t l : c ) b.clauses.push_back( Literal::fromDimacs( l ) );
        b.segments.push_back( { begin, b.clauses.size( ) } );
    }
    return b;
}

/// waerden(3,3;n): for every arithmetic progression i, i+d, i+2d in 1..=n, the
/// clauses (x_i x_{i+d} x_{i+2d}) and (¬x_i ¬x_{i+d} ¬x_{i+2d}). All positive
/// clauses come first, then all negative ones.
inline auto
waerden33( const BenchInfo *info ) -> Bench
{
    auto n = static_cast<ssize_t>( info->num_vars );

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
    Bench b{ *info, { }, { } };
    b.clauses.reserve( 2 * 3 * aps.size( ) );
    b.segments.reserve( 2 * aps.size( ) );
    for ( auto new_lit : { Literal::newPos, Literal::newNeg } ) {
        for ( auto &ap : aps ) {
            size_t begin = b.clauses.size( );
            for ( ssize_t v : ap ) b.clauses.push_back( new_lit( v ) );
            b.segments.push_back( { begin, b.clauses.size( ) } );
        }
    }
    return b;
}

/// langford(n): place two 1s, two 2s, ..., two n's into 2n slots so that
/// exactly k slots intervene between the two appearances of k, for each k.
///
/// Variable x_v means "the two k's go into slots i and i+k+1", for every k in
/// 1..=n and i in 1..=2n-k-1, numbered in that (k, then i) order. To break the
/// left-right symmetry (reversing a solution gives another solution), the
/// final option of the final digit, k = n at i = n-1 (touching the right
/// boundary), is removed: its reversal is k = n at i = 1, which is kept. So
/// there are 3n(n-1)/2 - 1 variables. Each digit k
/// and each slot s must be covered exactly once; for each of these 3n
/// constraints, one clause says "at least one" (all its variables positive)
/// and binary clauses say "at most one" (¬x_u ¬x_v for every pair u < v). All
/// digit constraints come first, then all slot constraints.
inline auto
langford( const BenchInfo *info, ssize_t n ) -> Bench
{
    // Stage 1: enumerate all placements; record which vars cover each digit
    // and each slot. Both `digits` and `slots` are 1-based (index 0 unused):
    // `digits[k]` for k in 1..=n and `slots[s]` for s in 1..=2n.
    //
    // This is Eq(11) on Page 189.
    std::vector<std::vector<ssize_t>> digits( static_cast<size_t>( n + 1 ) );
    std::vector<std::vector<ssize_t>> slots( static_cast<size_t>( 2 * n + 1 ) );
    ssize_t                           v = 0;
    for ( ssize_t k = 1; k <= n; k++ ) {
        for ( ssize_t i = 1; i <= 2 * n - k - 1; i++ ) {
            if ( k == n && i == n - 1 ) continue;  // Symmetry breaking.
            v++;
            digits[static_cast<size_t>( k )].push_back( v );
            slots[static_cast<size_t>( i )].push_back( v );
            slots[static_cast<size_t>( i + k + 1 )].push_back( v );
        }
    }
    if ( static_cast<size_t>( v ) != info->num_vars )
        TAOCP_EVAL_PANIC( "langford(%zd) has %zd vars, info says %zu", n, v,
                          info->num_vars );

    // Stage 2: emit the exactly-one clauses of each constraint.
    //
    // This is Eq(11) on Page 189.
    Bench b{ *info, { }, { } };
    auto  exactlyOne = [&b]( const std::vector<ssize_t> &vs ) {
        size_t begin = b.clauses.size( );
        for ( ssize_t u : vs ) b.clauses.push_back( Literal::newPos( u ) );
        b.segments.push_back( { begin, b.clauses.size( ) } );
        for ( size_t a = 0; a < vs.size( ); a++ ) {
            for ( size_t c = a + 1; c < vs.size( ); c++ ) {
                begin = b.clauses.size( );
                b.clauses.push_back( Literal::newNeg( vs[a] ) );
                b.clauses.push_back( Literal::newNeg( vs[c] ) );
                b.segments.push_back( { begin, b.clauses.size( ) } );
            }
        }
    };
    for ( ssize_t k = 1; k <= n; k++ )
        exactlyOne( digits[static_cast<size_t>( k )] );
    for ( ssize_t s = 1; s <= 2 * n; s++ )
        exactlyOne( slots[static_cast<size_t>( s )] );
    return b;
}

}  // namespace detail

inline auto
Bench::get( ID id ) -> Bench
{
    switch ( id ) {
    case ID::Rivest: {
        BenchInfo info{
            /*vol=*/"4b",
            /*page=*/"188",
            /*eqn=*/"6",
            /*name=*/"rivest",
            /*desc=*/"The shortest interesting formula in 3CNF.",
            /*num_vars=*/4,
            /*satisfiable=*/false,
        };
        std::initializer_list<std::initializer_list<ssize_t>> cs{
            { 1,  2, -3},
            { 2,  3, -4},
            { 3,  4,  1},
            { 4, -1,  2},
            {-1, -2,  3},
            {-2, -3,  4},
            {-3, -4, -1},
            {-4,  1, -2},
        };
        return detail::fromClauses( &info, &cs );
    }
    case ID::Rivest7: {
        // Rivest's clauses (6) without the last one, (¬x_4 x_1 ¬x_2).
        BenchInfo info{
            /*vol=*/"4b",
            /*page=*/"188",
            /*eqn=*/"7",
            /*name=*/"rivest7",
            /*desc=*/"Rivest's clauses without the last one",
            /*num_vars=*/4,
            /*satisfiable=*/true,
        };
        std::initializer_list<std::initializer_list<ssize_t>> cs{
            { 1,  2, -3},
            { 2,  3, -4},
            { 3,  4,  1},
            { 4, -1,  2},
            {-1, -2,  3},
            {-2, -3,  4},
            {-3, -4, -1},
        };
        return detail::fromClauses( &info, &cs );
    }
    case ID::Waerden339: {
        BenchInfo info{
            /*vol=*/"4b",
            /*page=*/"188",
            /*eqn=*/"9",
            /*name=*/"waerden(3,3;9)",
            /*desc=*/
            "A binary sequence of length 9 with no 3 equally spaced 0's and "
            "no 3 equally spaced 1's.",
            /*num_vars=*/9,
            /*satisfiable=*/false,
        };
        return detail::waerden33( &info );
    }
    case ID::Langford3: {
        BenchInfo info{
            /*vol=*/"4b",
            /*page=*/"189",
            /*eqn=*/"11",
            /*name=*/"langford(3)",
            /*desc=*/
            "Place two 1s, two 2s and two 3s into 6 slots so that exactly k "
            "slots intervene between the two k's, for each k.",
            // One var per placement (i, i+k+1) of k, i in 1..=2n-k-1: 4 for
            // k=1, 3 for k=2, 2 for k=3. Minus 1 for (2,6) of k=3, dropped
            // by symmetry (its reversal (1,5) is kept): 4+3+2-1 = 8.
            /*num_vars=*/8,
            /*satisfiable=*/true,
        };
        return detail::langford( &info, 3 );
    }
    case ID::Langford9: {
        // Langford pairs exist only when n mod 4 is 0 or 3; 9 mod 4 = 1.
        BenchInfo info{
            /*vol=*/"4b",
            /*page=*/"219",
            /*eqn=*/"??",
            /*name=*/"langford(9)",
            /*desc=*/
            "Place two 1s, two 2s, ..., two 9s into 18 slots so that exactly "
            "k slots intervene between the two k's, for each k.",
            // One var per placement (i, i+k+1) of k, i in 1..=2n-k-1: 16 for
            // k=1 down to 8 for k=9, i.e. 3n(n-1)/2 = 108. Minus 1 for (8,18)
            // of k=9, dropped by symmetry (its reversal (1,11) is kept): 107.
            /*num_vars=*/107,
            /*satisfiable=*/false,
        };
        return detail::langford( &info, 9 );
    }
    case ID::Count:
        break;
    }
    TAOCP_EVAL_PANIC( "unknown bench id %d", static_cast<int>( id ) );
}

/// Prints the info of `bench` to stdout, plus its number of clauses and its
/// table size (the total number of literals in `bench->clauses`).
inline auto
printBenchInfo( const Bench *bench ) -> void
{
    const BenchInfo *info = &bench->info;
    ::printf( "BenchInfo {\n" );
    ::printf( "    vol: \"%s\",\n", info->vol );
    ::printf( "    page: \"%s\",\n", info->page );
    ::printf( "    eqn: \"%s\",\n", info->eqn );
    ::printf( "    name: \"%s\",\n", info->name );
    ::printf( "    desc: \"%s\",\n", info->desc );
    ::printf( "    num_vars: %zu,\n", info->num_vars );
    ::printf( "    satisfiable: %s,\n", info->satisfiable ? "true" : "false" );
    ::printf( "    num_clauses: %zu,\n", bench->segments.size( ) );
    ::printf( "    table_size: %zu,\n", bench->clauses.size( ) );
    ::printf( "}\n" );
}

}  // namespace taocp::sat::eval
