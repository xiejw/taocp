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

    size_t p = l_.size( );
    for ( size_t j = 0; j < num_clauses_; j++ ) {
        auto &seg = ( *segments )[j];
        for ( size_t i = seg.begin; i < seg.end; i++ ) {
            l_[--p] = ( *clauses )[i];
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
