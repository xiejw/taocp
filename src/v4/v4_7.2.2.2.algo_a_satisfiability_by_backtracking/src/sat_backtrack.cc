// forge:v1
#include "sat_backtrack.h"

#include <stdio.h>

#include "log.h"

namespace taocp {

BacktrackSolver::BacktrackSolver( size_t                      num_vars,
                                  const std::vector<Literal> *clauses,
                                  const std::vector<Segment> *segments )
    : num_vars_( num_vars ), num_clauses_( segments->size( ) )
{
    /* Cells 2 .. 2n+1 are the list heads, one per literal; the clause cells
     * follow from 2n+2. Cells 0 and 1 are unused (1-based literals start at
     * 2). */
    l_.resize( 2 + 2 * num_vars_ + clauses->size( ) );

    /* Fill clauses from the end of l_ backwards: clause 1 occupies the
     * highest cells, then clause 2 just below it, and so on, so clause m
     * starts at cell 2n+2. Walking backwards while reading each clause
     * forward stores its literals in reverse order. */
    start_.resize( 1 + num_clauses_ );
    size_.resize( 1 + num_clauses_ );

    /* F, B and C are the same size as L; resize zero-fills them. */
    f_.resize( l_.size( ) );
    b_.resize( l_.size( ) );
    c_.resize( l_.size( ) );

    /* Make each list head 2 .. 2n+1 an empty circular list: F and B point
     * back to the head itself. C(l) = 0 (no clauses yet) is already done by
     * the zero-fill above. */
    for ( size_t l = 2; l < 2 + 2 * num_vars_; l++ ) {
        f_[l] = l;
        b_[l] = l;
        /* c_[l] = 0; */
    }

    size_t p = l_.size( );
    for ( size_t j = 0; j < num_clauses_; j++ ) {
        auto &seg = ( *segments )[j];
        for ( size_t i = seg.begin; i < seg.end; i++ ) {
            Literal l = ( *clauses )[i];

            /* Each clause must list its literals in strictly ascending order
             * (no duplicates). Compare every literal with the one before it
             * in the same clause, and panic if it is not larger. */
            if ( i > seg.begin && ( *clauses )[i - 1] >= l ) {
                PANIC( "clause %zu is not sorted: literal %" PRI_LITERAL
                       " at %zu >= literal %" PRI_LITERAL " at %zu",
                       j + 1, ( *clauses )[i - 1], i - 1, l, i );
            }

            --p;

            /* Set up circular list */

            f_[p]     = l;
            f_[b_[l]] = p;

            b_[p] = b_[l];
            b_[l] = p;

            c_[l]++;
            l_[p] = l;
        }

        /* Clause ids are 1-based: segment j is clause j+1. Its cells are
         * START(j+1) .. START(j+1) + SIZE(j+1) - 1. */
        start_[j + 1] = p;
        size_[j + 1]  = seg.end - seg.begin;
    }
}

auto
BacktrackSolver::searchOneSolution( ) -> std::optional<std::vector<Literal>>
{
    return std::nullopt;
}

}  // namespace taocp
