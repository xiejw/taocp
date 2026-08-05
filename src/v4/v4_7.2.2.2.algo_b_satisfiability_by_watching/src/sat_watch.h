// vim: ft=cpp
// forge:v1
//
// Algorithm B: Satisfiability by Watching. Vol 4B Page 215.
#pragma once

#include <initializer_list>
#include <optional>
#include <vector>

#include "sat_literal.h"

namespace taocp {

/* === --- Defined type for literals --------------------------------------- ===
 */

class WatchSolver {
      private:
        size_t num_literals_;
        size_t num_clauses_;
        size_t num_emitted_clauses_;

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
        std::vector<literal_t> watch_;

        /*
         * LINK(j) is the pointer to the next clause with the same watched
         * literal or 0 if last such clause.
         *
         * Index by clause. 1-based.
         */
        std::vector<size_t> link_;

      public:
        /* === --- Constructors ----------------------------------------- === */

        /* For all constructors, num_literals and num_clauses are fixed and
         * cannot be changed anymore. num_reserved_cells should be the best
         * guess for the application. It requires one more space for the
         * placeholder. */
        WatchSolver( size_t num_literals, size_t num_clauses,
                     size_t num_reserved_cells );
        WatchSolver( size_t num_literals, size_t num_clauses );

        auto reserveCells( size_t num_cells ) -> void;

      public:
        /* === --- Conform Base Class ----------------------------------- === */
        void emitClause( size_t size, const literal_t * );
        void emitClause( std::initializer_list<literal_t> );
        auto searchOneSolution( ) -> std::optional<std::vector<literal_t>>;

      public:
        /* === --- A set of debug tooling. ----------------------------- === */

        /* Print the internal states of the solver. Orthogonal to debug mode. */
        auto dumpDebugInfo( ) -> void;

      private:
        /// Validate all literals are in right value domain.
        auto DebugCheck( size_t size, const literal_t * ) const -> void;
};
}  // namespace taocp
