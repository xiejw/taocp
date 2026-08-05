// forge:v1
#include "two_sat_solver.h"

#include <assert.h>

#define DEBUG 0
#define DEBUG_PRINTF \
        if ( DEBUG ) INFO

namespace taocp {

// Reserve 2n in the graph. n for the variables, n for the complements of
// variables.
TwoSatSolver::TwoSatSolver( size_t num_vars )
    : n_( num_vars ), g_{ 2 * num_vars }
{
}

void
TwoSatSolver::addKromClause( size_t v_id, size_t u_id )
{
        assert( v_id >= 0 && v_id < 2 * this->n_ );
        assert( u_id >= 0 && u_id < 2 * this->n_ );

        auto *comp_v = this->g_.getVertex( this->getComplementId( v_id ) );
        comp_v->arcs.push_back( this->g_.getVertex( u_id ) );

        auto *comp_u = this->g_.getVertex( this->getComplementId( u_id ) );
        comp_u->arcs.push_back( this->g_.getVertex( v_id ) );
}

bool
TwoSatSolver::checkSatisfiability( )
{
        this->g_.runAlgoT( );
        auto ids = g_.getComponentIdsAfterAlgoT( );

        if ( DEBUG ) {
                DEBUG_PRINTF( "\nN= %d\n", int( this->n_ ) );
                for ( size_t i = 0; i < ids->size( ); i++ ) {
                        DEBUG_PRINTF( "%d => comp %d", int( i ),
                                      int( ids->at( i ) ) );
                }
        }

        // Exercise 54 (Page 86): Instead of checking whether a variable and
        // its complement exist in one component for all variables, we could
        // simply check the first variable only. If its complement is in the
        // same component, the 2SAT is not satisfiable.
        const size_t        SENT = 2 * this->n_;
        std::vector<size_t> leader_for_components( ids->size( ), SENT );

        for ( size_t vertex_id = 0; vertex_id < SENT; vertex_id++ ) {
                const size_t component_id = ( *ids )[vertex_id];
                const size_t current_leader =
                    leader_for_components[component_id];

                DEBUG_PRINTF(
                    "CHECK component_id = %2d vertex_id = %2d current_leader = "
                    "%2d",
                    int( component_id ), int( vertex_id ),
                    int( current_leader ) );

                if ( current_leader == SENT ) {
                        leader_for_components[component_id] = vertex_id;
                        continue;
                }

                if ( this->getCanonicalId( current_leader ) ==
                     this->getCanonicalId( vertex_id ) ) {
                        return false;
                }
        }

        return true;
}
}  // namespace taocp
