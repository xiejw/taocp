// vim: ft=cpp
// forge:v1
//
// Algorithm D: Satisfiability by cyclic DPLL. Vol 4B Page 217.
#ifndef TAOCP_SRC_SAT_DPLL_H_
#define TAOCP_SRC_SAT_DPLL_H_

#include <optional>
#include <vector>

#include "sat_literal.h"

namespace taocp {

/// Clause j is `clauses[begin, end)` where `{begin, end} = segments[j]`.
struct Segment {
    size_t begin;
    size_t end;
};

class DpllSolver {
  private:
    size_t num_vars_;
    size_t num_clauses_;

    std::vector<size_t> cells_; /* Index by cell. */

    /*
     * The start position for each clause in cells.  The clause is
     * reversely ordered. And each literal is reversely ordered in the
     * clause. The watched literal is at the first position of the clause.

     *
     * Index by clause. 1-based.
     */
    std::vector<size_t> start_;

    /*
     * The start pointer for the clause watching the literal. The watched
     * literal is at the first position of the clause.
     *
     * Index by literal. 1-based.
     */
    std::vector<size_t> watch_;

    /*
     * LINK(j) is the pointer to the next clause with the same watched
     * literal or 0 if last such clause.
     *
     * Index by clause. 1-based.
     */
    std::vector<size_t> link_;

  public:
    /* === --- Constructors ----------------------------------------- === */

    /* All clauses are given at once, laid out like `eval::Bench`: clause j
     * is `(*clauses)[begin, end)` where `{begin, end} = (*segments)[j]`.
     * Variables are 1-based in [1, num_vars]. The first literal of each
     * clause is the one initially watched. The solver copies what it needs;
     * it does not keep the pointers. */
    DpllSolver( size_t num_vars, const std::vector<Literal> *clauses,
                const std::vector<Segment> *segments );

  public:
    /* === --- Search ---------------------------------------------- === */
    auto searchOneSolution( ) -> std::optional<std::vector<Literal>>;
};
}  // namespace taocp

#endif  // TAOCP_SRC_SAT_DPLL_H_
