// forge:v1
#include "sat_dpll.h"

#include <assert.h>
#include <stdio.h>

#include "log.h"

namespace taocp {

DpllSolver::DpllSolver( size_t num_vars, const std::vector<Literal> *clauses,
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
DpllSolver::searchOneSolution( ) -> std::optional<std::vector<Literal>>
{
    /* === --- This algorithm is Vol 4b, Page 217. ------------------ === */

D1:  // Initialize
    size_t               d = 0;
    size_t               h = 0;
    size_t               t = 0;
    size_t               n = num_vars_;
    std::vector<size_t>  m( n + 1 );
    std::vector<ssize_t> x( n + 1 );
    std::vector<size_t>  next( n + 1 );
    std::vector<size_t>  h_arr( n + 1 );

    for ( size_t k = n; k >= 1; k-- ) {
        x[k] = -1;  // unset value
        if ( watch_[2 * k] != 0 || watch_[2 * k + 1] != 0 ) {
            next[k] = h;
            h       = k;
            if ( t == 0 ) t = k;
        }
    }

    if ( t != 0 ) {  // Complete the active ring.
        next[t] = h;
    }

    size_t k;  // Track the current position in the active ring.

D2:  // Success?
    if ( t == 0 ) goto happy_exit;

    k = t;

D3:  // Look for unit clauses
    while ( 1 ) {
        h = next[k];

        // Calcuate f.
        //
        // It is a hand coded jump table to avoid calling a function side a hot
        // loop.
        //
        // This is Answer of Ex 129 on Page 574.
        size_t f_pos, f_neg;
        size_t j;
        {
            Literal l = 2 * h;
            size_t  f;
            goto f_func;
        f_calc:
            if ( ( l & 1 ) == 0 ) {
                f_pos = f;
                l++;
                goto f_func;
            } else {
                f_neg = f;
                goto f_end;
            }

        f_func:
            j = watch_[l];  // clause id
            while ( j != 0 ) {
                // Inner loop of clause j.
                size_t p = start_[j] + 1;
                while ( 1 ) {
                    if ( p == start_[j - 1] ) goto subroutine_f_1;
                    size_t l_p = cells_[p];
                    size_t var = l_p >> 1;
                    if ( x[var] == ( l_p & 1 ) ) {  // l_p is false
                        p++;
                        continue;
                    }
                    break;
                }

                j = link_[j];
            }
            f = 0;
            goto subroutine_f_exit;

        subroutine_f_1:
            f = 1;
        subroutine_f_exit:
            goto f_calc;

        f_end:
            (void)0;
        }

        size_t f = f_pos + 2 * f_neg;
        assert( f <= 3 );

        if ( f == 3 ) goto D7;

        if ( f > 0 ) {
            m[d + 1] = f + 3;
            t        = k;  // Mark ring end and continue from that next time.
            goto D5;       // h is the var to consider next.
        }

        // Continue the active ring if not ended.
        if ( h != t ) {
            k = h;
            continue;
        } else {
            break;
        }
    }

D4:                      // Two-way branch
    h        = next[t];  // h is the var to consider next.
    m[d + 1] = ( watch_[2 * h] == 0 || watch_[2 * h + 1] != 0 );

D5:  // Move on

    // h is the var to consider.
    d++;
    k        = h;
    h_arr[d] = h;

    if ( t == k ) {
        t = 0;  // Right is empty now.
    } else {
        // Delete k from ring.
        h       = next[k];
        next[t] = h;
    }

D6:  // Update watches

{
    size_t b = ( m[d] + 1 ) % 2;
    x[k]     = ssize_t( b );  // -1 is allowed so ssize_t

    {  // This is the answer to Ex 130 on Page 574
        Literal l = 2 * k + b;
        size_t  j = watch_[l];  // clause id
        watch_[l] = 0;
        while ( j != 0 ) {
            // (i)
            size_t next_j = link_[j];
            size_t i      = start_[j];
            size_t p      = i + 1;

            // (ii)
            while ( 1 ) {
                size_t l_p = cells_[p];
                size_t var = l_p >> 1;
                if ( x[var] == ( l_p & 1 ) ) {  // l_p is false
                    p++;
                } else {
                    break;
                }
            }
            assert( p < start_[j - 1] );  // TODO why?

            // (iii)
            size_t next_watch = cells_[p];
            cells_[p]         = l;
            cells_[i]         = next_watch;

            // (iv)
            p        = watch_[next_watch];
            size_t q = watch_[( next_watch ^ 1 )];
            if ( p != 0 || q != 0 || x[next_watch >> 1] >= 0 ) {
                goto vi;
            }

            // (v)
            // insert next_watch into right
            if ( t == 0 ) {
                h       = ( next_watch >> 1 );
                t       = h;
                next[t] = h;
            } else {
                next[( next_watch >> 1 )] = h;
                h                         = ( next_watch >> 1 );
                next[t]                   = h;
            }

            // (vi)
        vi:  // insert j into watch list of next_watch.
            link_[j]           = p;
            watch_[next_watch] = j;

            // (vii)
            j = next_j;
        }
    }
}

    goto D2;

D7:  // Backtrack
    t = k;
    while ( m[d] >= 2 ) {
        k    = h_arr[d];
        x[k] = -1;

        if ( watch_[2 * k] != 0 || watch_[2 * k + 1] != 0 ) {
            // Put k back to ring.
            next[k] = h;
            h       = k;
            next[t] = h;
        }
        d--;
    }
    assert( d >= 0 );  // m[d]==0 so the loop will stop.

D8:  // Failure?
    assert( m[d] < 2 );
    if ( d > 0 ) {
        m[d] = 3 - m[d];
        k    = h_arr[d];
        goto D6;
    }

    return std::nullopt;

happy_exit:
    //  Translate m to results;
    std::vector<Literal> result( num_vars_ );

    // There are vars never in ring, so any value is ok
    for ( size_t k = 1; k <= num_vars_; k++ )
        result[k - 1] = complement( literal( k ) );  // free vars: arbitrary

    // Now fill with vars in ring.
    for ( size_t i = 1; i <= num_vars_; i++ ) {
        assert( m[i] <= 5 );          // See meaning on Page 218.
        size_t result_id = h_arr[i];  // this is indirection.

        assert( result_id >= 0 && result_id <= num_vars_ );
        if ( result_id == 0 ) {
            continue;  // never in right. value in result is set above.
        }
        if ( m[i] & 1 ) {
            result[result_id - 1] = complement( literal( result_id ) );
        } else {
            result[result_id - 1] = literal( result_id );
        }
    }
    return result;
}

}  // namespace taocp
