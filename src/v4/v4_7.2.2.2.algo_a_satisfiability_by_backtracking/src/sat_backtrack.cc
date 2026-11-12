// forge:v1
#include "sat_backtrack.h"

#include <stdio.h>

#include "log.h"

namespace taocp {

namespace {

constexpr bool kDebugMode = false;

auto
debugCheck( size_t num_vars, size_t size, const Literal *lits ) -> void
{
    for ( size_t x = 0; x < size; x++ ) {
        auto lit = lits[x];
        auto var = decodeVariable( lit );
        if ( var > num_vars ) {
            PANIC( "lit" );
        }
        if ( var < 1 ) {
            PANIC( "lit < 1" );
        }
    }
}
}  // namespace

BacktrackSolver::BacktrackSolver( size_t num_vars, size_t num_clauses,
                          size_t num_reserved_cells )
    : num_vars_( num_vars ),
      num_clauses_( num_clauses ),
      num_emitted_clauses_( 0 ),
      start_( 1 + num_clauses ),
      backtrack_( 2 + 2 * num_vars ),
      link_( 1 + num_clauses )
{
    cells_.reserve( 1 + num_reserved_cells );
    cells_.push_back( 0 );
}

BacktrackSolver::BacktrackSolver( size_t num_vars, size_t num_clauses )
    : BacktrackSolver( num_vars, num_clauses, /*a guess*/ 5 * num_clauses )
{
}

auto
BacktrackSolver::reserveCells( size_t num_cells ) -> void
{
    cells_.reserve( 1 + num_cells );
}

auto
BacktrackSolver::emitClause( std::initializer_list<Literal> lits ) -> void
{
    this->emitClause( lits.size( ), std::data( lits ) );
}

auto
BacktrackSolver::emitClause( size_t size, const Literal *lits ) -> void
{
    /* === --- Few quick sanity checks. ----------------------------- === */
    if ( size == 0 ) PANIC( "emitted clause cannot be empty." );
    if ( num_emitted_clauses_ >= num_clauses_ )
        PANIC( "emitted clause is full. Cannot submit one more." );

    if ( kDebugMode ) debugCheck( this->num_vars_, size, lits );

    /* Clause id is 1-based, and decreasing order. */
    auto clause_id = num_clauses_ - num_emitted_clauses_;

    /* Put each literal into the internal data structures. */
    bool is_first = true;
    for ( size_t x = 0; x < size; x++ ) {
        auto lit = lits[x];

        /* Literal is already encoded as 2x or 2x+1 (see sat_literal.h). */
        auto encoded_lit = lit;
        cells_.push_back( encoded_lit );

        if ( !is_first ) continue;

        /* === --- Special block to handle the first literal. ------ ===
         *
         * Start, Backtrack, Link should be recorded correctly.
         */
        is_first = false;

        /* Update the start array. */
        auto pos          = cells_.size( );
        start_[clause_id] = pos - 1;

        /* Update the backtrack list. */
        if ( backtrack_[encoded_lit] == 0 ) {
            // This branch seems is identical to next.
            backtrack_[encoded_lit] = clause_id;
        } else {
            link_[clause_id]    = backtrack_[encoded_lit];
            backtrack_[encoded_lit] = clause_id;
        }
    }

    num_emitted_clauses_++;

    if ( num_emitted_clauses_ == num_clauses_ ) {
        /* Once all clauses are emitted, the start[0] - 1 should
         * point to the final cell. */
        start_[0] = cells_.size( );
    }
}

auto
BacktrackSolver::searchOneSolution( ) -> std::optional<std::vector<Literal>>
{
    if ( num_emitted_clauses_ != num_clauses_ ) {
        PANIC( "emitted clauses are not enough. expected {}, got {}",
               (size_t)num_clauses_, (size_t)num_emitted_clauses_ );
    }
    /* === --- This algorithm is Vol 4b, Page 215. ------------------ === */

B1:  // Init
    size_t d = 1;
    size_t n = num_vars_;

    std::vector<size_t> m( n + 1 );

B2:  // Rejoice or choose
    while ( d <= n ) {
        m[d] = backtrack_[2 * d] == 0 || backtrack_[2 * d + 1] != 0;
        // size_t l      = 2 * d + m[d];
        size_t comp_l = 2 * d + ( m[d] ^ 1 );

    B3:
        /* B3 Remove C(l) if possible. Page 573. Ex 124 */
        size_t j = backtrack_[comp_l];

        if ( kDebugMode )
            INFO(
                "work at B3 at level d %zu to remove complement of "
                "literal with starting clause %zu (comp l = %zu)",
                d, j, comp_l );

        // j tracks the current clause backtracking the comp_l
        while ( j != 0 ) {
            if ( kDebugMode ) {
                INFO(
                    "--> sub level B3 to work on clause j = "
                    "%zu",
                    j );
            }
            /* A literal other than comp_l should be backtracked in
             * clause j. */

            size_t begin  = start_[j];
            size_t end    = start_[j - 1];  // start_[0] is valid.
            size_t next_j = link_[j];

            if ( kDebugMode ) {
                if ( next_j > num_clauses_ ) {
                    PANIC( "wrong next j" );
                }
            }

            size_t k = begin + 1;
            for ( ; k < end; k++ ) {
                if ( kDebugMode )
                    INFO(
                        "--> --> sub level B3 clause j = "
                        "%zu work on cell %zu (begin %zu, "
                        "end %zu)",
                        j, k, begin, end );

                size_t new_l = cells_[k];

                /* If new_l isn't false. Swap it to
                 * beginning. */
                if ( ( new_l >> 1 ) > d ||
                     ( ( new_l + m[new_l >> 1] ) % 2 ) == 0 ) {
                    cells_[begin] = new_l;
                    cells_[k]     = comp_l;
                    link_[j]      = backtrack_[new_l];
                    backtrack_[new_l] = j;
                    j             = next_j;
                    break; /* This clause is done. */
                }
            }

            if ( k == end ) {
                /* Cannot stop backtracking on comp_l. */
                backtrack_[comp_l] = j;
                if ( kDebugMode )
                    INFO(
                        "cannot stop backtracking on "
                        "complement of literal, go to B5 "
                        "at level d %d",
                        (int)d );
                goto B5;
            }
        }

    B4:  // Advance
        backtrack_[comp_l] = 0;
        d++;
        if ( kDebugMode ) INFO( "advance to with B2 at level d %zu", d );
        continue; /* Return to B2 */

    B5:  // B5 Try again
        if ( m[d] < 2 ) {
            size_t new_md = 3 - m[d];
            m[d]          = new_md;
            // l             = 2 * d + ( new_md & 1 );
            comp_l = 2 * d + ( ( new_md & 1 ) ^ 1 );

            if ( kDebugMode )
                INFO(
                    "try again with B3 at level d %zu and m[d] "
                    "= %zu",
                    d, new_md );
            goto B3;
        }

    B6:  // Backtrack

        if ( d == 1 ) return std::nullopt; /* Failed */

        /* Otherwise */
        d--;

        if ( kDebugMode ) INFO( "backtrack with B5 at level d %zu", d );

        goto B5;

    }  // End of B2

    // Translate m to results;
    std::vector<Literal> result( num_vars_ );

    for ( size_t i = 1; i <= num_vars_; i++ ) {
        if ( m[i] & 1 ) {
            result[i - 1] = complement( literal( i ) );
        } else {
            result[i - 1] = literal( i );
        }
    }

    return result; /* Exit happily */
}

}  // namespace taocp
