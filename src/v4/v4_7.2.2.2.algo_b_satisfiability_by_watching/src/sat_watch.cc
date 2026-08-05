// forge:v1
#include "sat_watch.h"

#include <stdio.h>

#include "log.h"

#define DEBUG_MODE 0

namespace taocp {

WatchSolver::WatchSolver( size_t num_literals, size_t num_clauses,
                          size_t num_reserved_cells )
    : num_literals_( num_literals ),
      num_clauses_( num_clauses ),
      num_emitted_clauses_( 0 ),
      start_( 1 + num_clauses ),
      watch_( 2 + 2 * num_literals ),
      link_( 1 + num_clauses )
{
        cells_.reserve( 1 + num_reserved_cells );
        cells_.push_back( 0 );
}

WatchSolver::WatchSolver( size_t num_literals, size_t num_clauses )
    : WatchSolver( num_literals, num_clauses, /*a guess*/ 5 * num_clauses )
{
}

auto
WatchSolver::reserveCells( size_t num_cells ) -> void
{
        cells_.reserve( 1 + num_cells );
}

auto
WatchSolver::emitClause( std::initializer_list<literal_t> lits ) -> void
{
        this->emitClause( lits.size( ), std::data( lits ) );
}

auto
WatchSolver::emitClause( size_t size, const literal_t *lits ) -> void
{
        /* === --- Few quick sanity checks. ----------------------------- === */
        if ( size == 0 ) PANIC( "emitted clause cannot be empty." );
        if ( num_emitted_clauses_ >= num_clauses_ )
                PANIC( "emitted clause is full. Cannot submit one more." );

        this->DebugCheck( size, lits );

        /* Clause id is 1-based, and decreasing order. */
        auto clause_id = num_clauses_ - num_emitted_clauses_;

        /* Put each literal into the internal data structures. */
        bool first_v = true;
        for ( size_t x = 0; x < size; x++ ) {
                auto lit = lits[x];

                /* For literal 'l', the value put into the cell is 2*l+C(l).
                 */
                auto raw_v     = DecodeRawLiteralValue( lit );
                auto is_c      = IsLiteralComplement( lit );
                auto literal_v = raw_v * 2 + ( is_c ? 1 : 0 );
                cells_.push_back( literal_v );

                if ( !first_v ) continue;

                /* === --- Special block to handle the first literal. ------ ===
                 *
                 * Start, Watch, Link should be recorded correctly.
                 */
                first_v = false;

                /* Update the start array. */
                auto pos          = cells_.size( );
                start_[clause_id] = pos - 1;

                /* Update the watch list. */
                if ( watch_[literal_v] == 0 ) {
                        // This branch seems is identical to next.
                        watch_[literal_v] = clause_id;
                } else {
                        link_[clause_id]  = watch_[literal_v];
                        watch_[literal_v] = clause_id;
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
WatchSolver::searchOneSolution( ) -> std::optional<std::vector<literal_t>>
{
        if ( num_emitted_clauses_ != num_clauses_ ) {
                PANIC( "emitted clauses are not enough. expected {}, got {}",
                       (size_t)num_clauses_, (size_t)num_emitted_clauses_ );
        }
        /* === --- This algorithm is Vol 4b, Page 215. ------------------ === */

B1:  // Init
        size_t d = 1;
        size_t n = num_literals_;

        std::vector<size_t> m( n + 1 );

B2:  // Rejoice or choose
        while ( d <= n ) {
                m[d] = watch_[2 * d] == 0 || watch_[2 * d + 1] != 0;
                // size_t l      = 2 * d + m[d];
                size_t comp_l = 2 * d + ( m[d] ^ 1 );

        B3:
                /* B3 Remove C(l) if possible. Page 573. Ex 124 */
                size_t j = watch_[comp_l];

                if ( DEBUG_MODE )
                        INFO(
                            "work at B3 at level d %zu to remove compliment of "
                            "literal with starting clause %zu (comp l = %zu)",
                            d, j, comp_l );

                // j tracks the current clause wathcing the comp_l
                while ( j != 0 ) {
                        if ( DEBUG_MODE ) {
                                INFO(
                                    "--> sub level B3 to work on clause j = "
                                    "%zu",
                                    j );
                                dumpDebugInfo( );
                        }
                        /* A literal other than comp_l should be watched in
                         * clause j. */

                        size_t begin  = start_[j];
                        size_t end    = start_[j - 1];  // start_[0] is valid.
                        size_t next_j = link_[j];

                        if ( DEBUG_MODE ) {
                                if ( next_j > num_clauses_ ) {
                                        dumpDebugInfo( );
                                        PANIC( "wrong next j" );
                                }
                        }

                        size_t k = begin + 1;
                        for ( ; k < end; k++ ) {
                                if ( DEBUG_MODE )
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
                                if ( DEBUG_MODE )
                                        INFO(
                                            "cannot stop watching on "
                                            "compliment of literal, go to B5 "
                                            "at level d %d",
                                            (int)d );
                                goto B5;
                        }
                }

        B4:  // Advance
                watch_[comp_l] = 0;
                d++;
                if ( DEBUG_MODE )
                        INFO( "advance to with B2 at level d %zu", d );
                continue; /* Return to B2 */

        B5:  // B5 Try again
                if ( m[d] < 2 ) {
                        size_t new_md = 3 - m[d];
                        m[d]          = new_md;
                        // l             = 2 * d + ( new_md & 1 );
                        comp_l = 2 * d + ( ( new_md & 1 ) ^ 1 );

                        if ( DEBUG_MODE )
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

                if ( DEBUG_MODE ) INFO( "backtrack with B5 at level d %zu", d );

                goto B5;

        }  // End of B2

        // Translate m to results;
        std::vector<literal_t> result( num_literals_ );

        for ( size_t i = 1; i <= num_literals_; i++ ) {
                if ( m[i] & 1 ) {
                        result[i - 1] = C( i );
                } else {
                        result[i - 1] = i;
                }
        }

        return result; /* Exit happily */
}

auto
WatchSolver::dumpDebugInfo( ) -> void
{
        //        /* === --- Internal
        //        --------------------------------------------- === */
        //        std::print( "internal\n" );
        //        std::print( "  num_literals {:3}\n", num_literals_ );
        //        std::print( "  num_clauses  {:3}\n", num_clauses_ );
        //
        //        /* === --- Cells
        //        ------------------------------------------------ === */
        //        std::print( "cells\n" );
        //        for ( size_t i = 0; i < cells_.size( ); i++ ) {
        //                std::print( "{:02} ", i );
        //        }
        //        std::print( "\n" );
        //        for ( size_t i = 0; i < cells_.size( ); i++ ) {
        //                std::print( "{:02} ", cells_[i] );
        //        }
        //        std::print( "\n" );
        //
        //        /* === --- Start
        //        ------------------------------------------------ === */
        //        std::print( "start\n" );
        //        for ( size_t i = 0; i < start_.size( ); i++ ) {
        //                std::print( "{:02} ", i );
        //        }
        //        std::print( "\n" );
        //        for ( size_t i = 0; i < start_.size( ); i++ ) {
        //                std::print( "{:02} ", start_[i] );
        //        }
        //        std::print( "\n" );
        //
        //        /* === --- Watch
        //        ------------------------------------------------ === */
        //        std::print( "watch\n" );
        //        for ( size_t i = 0; i < watch_.size( ); i++ ) {
        //                std::print( "{:02} ", i );
        //        }
        //        std::print( "\n" );
        //        for ( size_t i = 0; i < watch_.size( ); i++ ) {
        //                std::print( "{:02} ", watch_[i] );
        //        }
        //        std::print( "\n" );
        //
        //        /* === --- Link
        //        ------------------------------------------------- === */
        //        std::print( "link\n" );
        //        for ( size_t i = 0; i < link_.size( ); i++ ) {
        //                std::print( "{:02} ", i );
        //        }
        //        std::print( "\n" );
        //        for ( size_t i = 0; i < link_.size( ); i++ ) {
        //                std::print( "{:02} ", link_[i] );
        //        }
        //        std::print( "\n" );
}

auto
WatchSolver::DebugCheck( size_t size, const literal_t *lits ) const -> void
{
        for ( size_t x = 0; x < size; x++ ) {
                auto lit   = lits[x];
                auto raw_v = DecodeRawLiteralValue( lit );
                if ( raw_v > num_literals_ ) {
                        PANIC( "lit" );
                }
                if ( raw_v < 1 ) {
                        PANIC( "lit < 1" );
                }
        }
}

}  // namespace taocp
