// vim: ft=cpp
// forge:v2
//
// Benchmark problems for evaluating SAT solvers. Header-only.
//
// Environment variables:
//
//   TAOCP_SAT_EVAL_ID  If set, `getAllIDs()` returns only the benchmark at
//                      this 0-based index in `enum class ID` (declaration
//                      order), instead of all of them. For example,
//                      `TAOCP_SAT_EVAL_ID=1 ./.build/main` runs only
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
    Waerden339,
    Count,
};

/// The number of benchmark problems.
inline constexpr size_t kNumIDs = static_cast<size_t>( ID::Count );

/// Returns every benchmark ID, in declaration order.
///
/// If the environment variable `TAOCP_SAT_EVAL_ID` is set, returns only the ID
/// whose index (0-based, in declaration order) it names, e.g.
/// `TAOCP_SAT_EVAL_ID=1` for `ID::Waerden339`. Panics if the value is not an
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
    size_t      num_vars;
};

/// Clause j is `clauses[begin, end)` where `{begin, end} = segments[j]`.
struct Segment {
    size_t begin;
    size_t end;
};

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
            /*num_vars=*/4,
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
    case ID::Waerden339: {
        BenchInfo info{
            /*vol=*/"4b",
            /*page=*/"188",
            /*eqn=*/"9",
            /*name=*/"waerden(3,3;9)",
            /*num_vars=*/9,
        };
        return detail::waerden33( &info );
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
    ::printf( "    num_vars: %zu,\n", info->num_vars );
    ::printf( "    num_clauses: %zu,\n", bench->segments.size( ) );
    ::printf( "    table_size: %zu,\n", bench->clauses.size( ) );
    ::printf( "}\n" );
}

}  // namespace taocp::sat::eval
