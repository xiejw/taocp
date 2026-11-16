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

            /* Variables are 1-based, so the variable of l must be in
             * 1 .. n. Otherwise l would fall outside the list heads
             * 2 .. 2n+1. */
            size_t var = decodeVariable( l );
            if ( var < 1 || var > num_vars_ ) {
                PANIC( "clause %zu has literal %" PRI_LITERAL
                       " with variable %zu out of range 1 .. %zu",
                       j + 1, l, var, num_vars_ );
            }

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
            c_[p] = j + 1;  // clause id 1-based.
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
    /* moves[d] for d = 1 .. n (1-based); moves[0] is unused. */
    std::vector<size_t> moves( num_vars_ + 1 );

A1:  // Initialize
    size_t m = num_clauses_;
    size_t a = m;
    size_t d = 1;

A2:  // Choose
    Literal l = 2 * d;
    if ( c_[l] <= c_[l + 1] ) l++;
    moves[d] = ( l & 1 ) + 4 * ( c_[l ^ 1] == 0 );

    if ( c_[l] == a ) {
        /* Terminate happily. Variable v is true (literal 2v) when moves[v] is
         * even, and false (literal 2v+1) when it is odd. */
        std::vector<Literal> solution;
        solution.reserve( num_vars_ );
        for ( size_t v = 1; v <= num_vars_; v++ ) {
            Literal lit = literal( v );
            solution.push_back( moves[v] % 2 == 0 ? lit : complement( lit ) );
        }
        return solution;
    }

A3:  // Remove -l
{
    Literal comp_l = complement( l );

    bool empty_clause = false;
    for ( size_t c = f_[comp_l]; c != comp_l; c = f_[c] ) {
        if ( ( size_[c]-- ) == 1 ) empty_clause = true;
    }

    if ( empty_clause ) goto A5;
}
A4:  // Deactivate l's clauses
{
    // Suppress l's clausese
    for ( size_t p = f_[l]; p != l; p = f_[p] ) {
        size_t j = c_[p];  // clause id 1-based.
        size_t i = start_[p];
        for ( size_t s = i; s < i + size_[j] - 1; s++ ) {
            size_t q = f_[s];
            size_t r = b_[s];
            b_[q]    = r;
            f_[r]    = q;
            c_[l_[s]]--;
        }
    }

    a -= c_[l];
    d++;
    goto A2;
}
A5:  // Try again
    if ( moves[d] < 2 ) {
        moves[d] = 3 - moves[d];
        l        = 2 * d + ( moves[d] & 1 );
        goto A3;
    }
A6:  // Backtrack
    if ( d == 1 ) return std::nullopt;
    d--;
    l = 2 * d + ( moves[d] & 1 );
A7:  // Reactivate l's clauses
{
    a += c_[l];

    // Reactivate l's clausese
    for ( size_t p = b_[l]; p != l; p = b_[p] ) {
        size_t j = c_[p];  // clause id 1-based.
        size_t i = start_[p];
        for ( size_t s = i; s < i + size_[j] - 1; s++ ) {
            size_t q = f_[s];
            size_t r = b_[s];
            b_[q]    = s;
            f_[r]    = s;
            c_[l_[s]]++;
        }
    }
}
A8:  // Unremove -l
{
    Literal comp_l = complement( l );

    for ( size_t c = f_[comp_l]; c != comp_l; c = f_[c] ) {
        size_[c]++;
    }
    goto A5;
}
}

}  // namespace taocp
