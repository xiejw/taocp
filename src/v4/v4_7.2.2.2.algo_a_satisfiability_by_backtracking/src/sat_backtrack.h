// vim: ft=cpp
// forge:v1
//
// Algorithm B: Satisfiability by Backtracking. Vol 4B Page 215.
#ifndef TAOCP_SRC_SAT_BACKTRACK_H_
#define TAOCP_SRC_SAT_BACKTRACK_H_

#include <initializer_list>
#include <optional>
#include <vector>

#include "sat_literal.h"

namespace taocp {

class BacktrackSolver {
  private:
    size_t num_vars_;
    size_t num_clauses_;
    size_t num_emitted_clauses_;

    std::vector<size_t> cells_; /* Index by cell. */

    /*
     * The start position for each clause in cells.  The clause is
     * reversely ordered. And each literal is reversely ordered in the
     * clause. The backtracked literal is at the first position of the clause.

     *
     * Index by clause. 1-based.
     */
    std::vector<size_t> start_;

    /*
     * The start pointer for the clause backtracking the literal. The backtracked
     * literal is at the first position of the clause.
     *
     * Index by literal. 1-based.
     */
    std::vector<size_t> backtrack_;

    /*
     * LINK(j) is the pointer to the next clause with the same backtracked
     * literal or 0 if last such clause.
     *
     * Index by clause. 1-based.
     */
    std::vector<size_t> link_;

  public:
    /* === --- Constructors ----------------------------------------- === */

    /* For all constructors, num_vars and num_clauses are fixed and
     * cannot be changed anymore. num_reserved_cells should be the best
     * guess for the application. It requires one more space for the
     * placeholder. */
    BacktrackSolver( size_t num_vars, size_t num_clauses,
                 size_t num_reserved_cells );
    BacktrackSolver( size_t num_vars, size_t num_clauses );

    auto reserveCells( size_t num_cells ) -> void;

  public:
    /* === --- Clauses and Search ---------------------------------- === */
    void emitClause( size_t size, const Literal *lits );
    void emitClause( std::initializer_list<Literal> lits );
    auto searchOneSolution( ) -> std::optional<std::vector<Literal>>;
};
}  // namespace taocp

#endif  // TAOCP_SRC_SAT_BACKTRACK_H_
