// vim: ft=cpp
// forge:v1
//
// Runs a SAT solver over every benchmark in `eval.h` and reports the results
// in one consistent format. Header-only.
//
// Usage:
//
//   struct MyDelegator : taocp::sat::eval::Delegator {
//       auto name( ) const -> const char * override { return "Algorithm B"; }
//       auto onInit( const Bench *bench ) -> void override { ... }
//       auto onSearchOneSolution( ) -> Solution override { ... }
//   };
//
//   MyDelegator d;
//   taocp::sat::eval::runAll( &d );
#pragma once

#include <stdio.h>

#include <optional>
#include <vector>

#include "sat/eval.h"
#include "sat/timer.h"

namespace taocp::sat::eval {

/// ANSI escape for green text, used for the per-benchmark name line.
inline constexpr const char *kColorGreen = "\033[32m";

/// The result of `Delegator::onSearchOneSolution`. A struct, so more fields
/// (e.g. stats, MEMs) can be added later without changing the interface.
struct Solution {
    /// A satisfying assignment (one literal per variable), or nullopt if
    /// unsatisfiable.
    std::optional<std::vector<Literal>> solution_opt;
};

/// Implemented by each solver. The runner calls `onInit` then
/// `onSearchOneSolution` once per benchmark, on the same delegator.
struct Delegator {
    virtual ~Delegator( ) = default;

    /// Name shown in the banner, e.g. "Algorithm B: Satisfiability by
    /// Watching".
    virtual auto name( ) const -> const char * = 0;

    /// (Re)builds the solver for `bench` and emits all its clauses. Must
    /// discard any state from a previous benchmark. Timed.
    virtual auto onInit( const Bench *bench ) -> void = 0;

    /// Searches for one solution of the benchmark given to the last `onInit`.
    /// Timed.
    virtual auto onSearchOneSolution( ) -> Solution = 0;
};

namespace detail {

/// Prints `lits` as, e.g., `{ x1 ¬x2 x3 }`.
inline auto
printLiterals( const std::vector<Literal> *lits ) -> void
{
    ::printf( "{" );
    for ( auto lit : *lits ) {
        auto [var, pos] = lit.getVarAndSign( );
        ::printf( pos ? " x%zu" : " ¬x%zu", var );
    }
    ::printf( " }\n" );
}

/// Panics unless `sol` satisfies every clause of `bench`.
inline auto
verify( const Bench *bench, const std::vector<Literal> *sol ) -> void
{
    // 1-based: value[k] is 1 for x_k, -1 for ¬x_k, 0 if unassigned.
    std::vector<int> value( bench->info.num_vars + 1, 0 );
    for ( auto lit : *sol ) {
        auto [var, pos] = lit.getVarAndSign( );
        if ( var > bench->info.num_vars )
            TAOCP_EVAL_PANIC( "solution var x%zu out of range", var );
        value[var] = pos ? 1 : -1;
    }
    for ( size_t j = 0; j < bench->segments.size( ); j++ ) {
        auto &seg       = bench->segments[j];
        bool  satisfied = false;
        for ( size_t i = seg.begin; i < seg.end && !satisfied; i++ ) {
            auto [var, pos] = bench->clauses[i].getVarAndSign( );
            satisfied       = value[var] == ( pos ? 1 : -1 );
        }
        if ( !satisfied ) TAOCP_EVAL_PANIC( "clause %zu not satisfied", j );
    }
}

}  // namespace detail

/// Runs `d` on every benchmark in `getAllIDs()`.
inline auto
runAll( Delegator *d ) -> void
{
    for ( auto id : getAllIDs( ) ) {
        ::printf( "%s==== --- %s --- ===%s\n", kColorGreen, d->name( ),
                  kColorReset );
        auto bench = Bench::get( id );
        printBenchInfo( &bench.info );

        Timer timer;

        timer.start( );
        d->onInit( &bench );
        timer.stopAndReport( "onInit" );

        timer.start( );
        auto res = d->onSearchOneSolution( );
        timer.stopAndReport( "onSearchOneSolution" );

        if ( res.solution_opt ) {
            detail::verify( &bench, &res.solution_opt.value( ) );
            ::printf( "Satisfiable (verified):\n" );
            detail::printLiterals( &res.solution_opt.value( ) );
        } else {
            ::printf( "Unsatisfiable\n" );
        }
    }
}

}  // namespace taocp::sat::eval
