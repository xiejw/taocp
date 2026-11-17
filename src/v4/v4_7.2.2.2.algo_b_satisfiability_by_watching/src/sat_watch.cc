// forge:v1
#include "sat_watch.h"

#include <stdio.h>

#include "log.h"

namespace taocp {

namespace {
constexpr bool kDebugMode = false;
}  // namespace

WatchSolver::WatchSolver( size_t num_vars, const std::vector<Literal> *clauses,
                          const std::vector<Segment> *segments )
    : num_vars_( num_vars ),
      num_clauses_( segments->size( ) ),
      start_( 1 + num_clauses_ ),
      watch_( 2 + 2 * num_vars ),
      link_( 1 + num_clauses_ )
{
    /* Cell 0 is a placeholder; the clause cells follow from cell 1. */
    cells_.reserve( 1 + clauses->size( ) );
    cells_.push_back( 0 );

    /* Clause ids are 1-based and assigned in decreasing order: the first
     * clause in `segments` is clause m, the last one is clause 1. So clause
     * j occupies cells [START(j), START(j-1)), with START(0) one past the
     * final cell. */
    for ( size_t i = 0; i < num_clauses_; i++ ) {
        auto  &seg       = ( *segments )[i];
        size_t clause_id = num_clauses_ - i;

        if ( seg.begin >= seg.end ) PANIC( "clause %zu is empty.", clause_id );

        start_[clause_id] = cells_.size( );

        for ( size_t x = seg.begin; x < seg.end; x++ ) {
            /* Literal is already encoded as 2x or 2x+1 (see sat_literal.h).
             * Variables are 1-based, so the variable must be in 1 .. n. */
            Literal lit = ( *clauses )[x];
            size_t  var = decodeVariable( lit );
            if ( var < 1 || var > num_vars_ ) {
                PANIC( "clause %zu has literal %" PRI_LITERAL
                       " with variable %zu out of range 1 .. %zu",
                       clause_id, lit, var, num_vars_ );
            }
            cells_.push_back( lit );
        }

        /* The first literal is watched: push clause j onto its watch list. */
        Literal watched  = ( *clauses )[seg.begin];
        link_[clause_id] = watch_[watched];
        watch_[watched]  = clause_id;
    }

    /* START(0) - 1 points to the final cell. */
    start_[0] = cells_.size( );
}

auto
WatchSolver::searchOneSolution( ) -> std::optional<std::vector<Literal>>
{
    /* === --- This algorithm is Vol 4b, Page 215. ------------------ === */

B1:  // Init
    size_t d = 1;
    size_t n = num_vars_;

    std::vector<size_t> m( n + 1 );

B2:  // Rejoice or choose
    while ( d <= n ) {
        m[d] = watch_[2 * d] == 0 || watch_[2 * d + 1] != 0;
        // size_t l      = 2 * d + m[d];
        size_t comp_l = 2 * d + ( m[d] ^ 1 );

    B3:
        /* B3 Remove C(l) if possible. Page 573. Ex 124 */
        size_t j = watch_[comp_l];

        if ( kDebugMode )
            INFO(
                "work at B3 at level d %zu to remove complement of "
                "literal with starting clause %zu (comp l = %zu)",
                d, j, comp_l );

        // j tracks the current clause watching the comp_l
        while ( j != 0 ) {
            if ( kDebugMode ) {
                INFO(
                    "--> sub level B3 to work on clause j = "
                    "%zu",
                    j );
            }
            /* A literal other than comp_l should be watched in
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
                    link_[j]      = watch_[new_l];
                    watch_[new_l] = j;
                    j             = next_j;
                    break; /* This clause is done. */
                }
            }

            if ( k == end ) {
                /* Cannot stop watching on comp_l. */
                watch_[comp_l] = j;
                if ( kDebugMode )
                    INFO(
                        "cannot stop watching on "
                        "complement of literal, go to B5 "
                        "at level d %d",
                        (int)d );
                goto B5;
            }
        }

    B4:  // Advance
        watch_[comp_l] = 0;
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
