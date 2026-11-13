// vim: ft=cpp
// forge:v1
//
// Algorithm A: Satisfiability by Backtracking. Vol 4B Page 212.
#ifndef TAOCP_SRC_SAT_BACKTRACK_H_
#define TAOCP_SRC_SAT_BACKTRACK_H_

#include <optional>
#include <vector>

#include "sat_literal.h"

namespace taocp {

/// Clause j is `clauses[begin, end)` where `{begin, end} = segments[j]`.
struct Segment {
    size_t begin;
    size_t end;
};

class BacktrackSolver {
  private:
    size_t num_vars_;
    size_t num_clauses_;

    std::vector<size_t> l_; /* Index by cell. Starting with 2. */
    std::vector<size_t> f_; /* Index by cell. Starting with 2. */
    std::vector<size_t> b_; /* Index by cell. Starting with 2. */
    std::vector<size_t> c_; /* Index by cell. Starting with 2. */

    /*
     * The start position for each clause in cells.  The clause is
     * reversely ordered. And each literal is reversely ordered in the
     * clause.
     *
     * The size of each clause in cells.
     *
     * Index by clause. 1-based.
     */
    std::vector<size_t> start_;
    std::vector<size_t> size_;

  public:
    /* === --- Constructors ----------------------------------------- === */

    /* All clauses are given at once, laid out like `eval::Bench`: clause j
     * is `(*clauses)[begin, end)` where `{begin, end} = (*segments)[j]`.
     * Variables are 1-based in [1, num_vars]. The solver copies what it
     * needs; it does not keep the pointers. */
    BacktrackSolver( size_t num_vars, const std::vector<Literal> *clauses,
                     const std::vector<Segment> *segments );

  public:
    /* === --- Search ---------------------------------------------- === */
    auto searchOneSolution( ) -> std::optional<std::vector<Literal>>;
};
}  // namespace taocp

#endif  // TAOCP_SRC_SAT_BACKTRACK_H_
