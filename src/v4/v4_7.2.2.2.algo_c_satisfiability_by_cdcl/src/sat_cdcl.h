// vim: ft=cpp
// forge:v1
//
// Algorithm C: Satisfiability by CDCL. Vol 4B Page 252.
#pragma once

#include <initializer_list>
#include <optional>
#include <vector>

#include "sat_literal.h"

namespace taocp {

/* === --- Defined type for literals --------------------------------------- ===
 */

class CDCLSolver {
      private:
        literal_t num_literals_;
        size_t    num_clauses_;
        size_t    num_emitted_clauses_;

        std::vector<literal_t> mems_;   // K: mem loc. V: literal
        std::vector<literal_t> trail_;  // K: trail idx. V: literal

      public:
        /* === --- Constructors ----------------------------------------- === */

      public:
        /* === --- Conform Base Class ----------------------------------- === */
        auto emitClause( size_t size, const literal_t * ) -> void;
        auto emitClause( std::initializer_list<literal_t> ) -> void;
        auto searchOneSolution( ) -> std::optional<std::vector<literal_t>>;
};
}  // namespace taocp
