// vim: ft=cpp
// forge:v1
//
// Benchmark problems for evaluating SAT solvers.
#pragma once

#include <vector>

#include <stddef.h>
#include <sys/types.h>

namespace taocp::eval {

// === --- Literal --------------------------------------------------------- ===
//

/// A literal: `Literal{k}` is x_k and `Literal{-k}` is ¬x_k, with k >= 1.
struct Literal {
    ssize_t v;

    /// Returns the positive literal x_v. `v` must be positive.
    static auto newPos( ssize_t v ) -> Literal;

    /// Returns the negative literal ¬x_v. `v` must be positive.
    static auto newNeg( ssize_t v ) -> Literal;

    /// Parses DIMACS notation: `k` means x_k and `-k` means ¬x_k. `l` must
    /// be nonzero.
    static auto fromDimacs( ssize_t l ) -> Literal;

    auto operator==( const Literal &o ) const -> bool { return v == o.v; }
};

// === --- Bench and ID ---------------------------------------------------- ===
//

/// Identifies a benchmark problem.
enum class ID {
    Rivest,
    Waerden339,
};

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

/// Prints the bench info to stdout.
auto printBenchInfo( const BenchInfo &info ) -> void;

}  // namespace taocp::eval
