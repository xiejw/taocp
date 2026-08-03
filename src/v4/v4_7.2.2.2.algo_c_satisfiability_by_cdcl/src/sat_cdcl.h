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
        literal_t m_num_literals;
        size_t    m_num_clauses;
        size_t    m_num_emitted_clauses;

        std::vector<literal_t> m_mems;

      public:
        /* === --- Constructors ----------------------------------------- === */

      public:
        /* === --- Conform Base Class ----------------------------------- === */
        auto emitClause( size_t size, const literal_t * ) -> void;
        auto emitClause( std::initializer_list<literal_t> ) -> void;
        auto searchOneSolution( ) -> std::optional<std::vector<literal_t>>;
};
}  // namespace taocp
