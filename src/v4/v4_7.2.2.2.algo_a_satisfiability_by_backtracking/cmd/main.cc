// forge:skip
#include <stddef.h>
#include <string.h>

#include <algorithm>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include "sat/eval_runner.h"

#include "sat_backtrack.h"
#include "sat_literal.h"

using taocp::BacktrackSolver;
using taocp::complement;
using taocp::decodeVariable;
using taocp::isLiteralComplement;
using taocp::literal;
using taocp::Literal;
using taocp::Segment;

namespace eval = taocp::sat::eval;

/* `Segment` and `eval::Segment` are separate types, but with identical layout,
 * so the benchmark segments can be copied into the solver's with one memcpy.
 */
static_assert( std::is_trivially_copyable_v<Segment> );
static_assert( std::is_trivially_copyable_v<eval::Segment> );
static_assert( sizeof( Segment ) == sizeof( eval::Segment ) );
static_assert( offsetof( Segment, begin ) == offsetof( eval::Segment, begin ) );
static_assert( offsetof( Segment, end ) == offsetof( eval::Segment, end ) );

namespace {

/// Translates the eval literal `lit` (x_k or ¬x_k) into the solver's encoding.
auto
toSolverLiteral( eval::Literal lit ) -> Literal
{
    auto [var, pos] = lit.getVarAndSign( );
    return pos ? literal( var ) : complement( literal( var ) );
}

/// Translates the solver literal `lit` back into an eval literal.
auto
toEvalLiteral( Literal lit ) -> eval::Literal
{
    auto var = static_cast<ssize_t>( decodeVariable( lit ) );
    return isLiteralComplement( lit ) ? eval::Literal::newNeg( var )
                                      : eval::Literal::newPos( var );
}

struct BacktrackDelegator : eval::Delegator {
    auto name( ) const -> const char * override
    {
        return "Algorithm B: Satisfiability by Backtracking";
    }

    auto onInit( const eval::Bench *bench ) -> void override
    {
        /* Copy the literals segment by segment, at the same indices, so the
         * memcpy'd segments below stay valid. The literals of each clause are
         * then sorted in ascending order. */
        std::vector<Literal> clauses( bench->clauses.size( ) );
        for ( auto &seg : bench->segments ) {
            for ( size_t i = seg.begin; i < seg.end; i++ )
                clauses[i] = toSolverLiteral( bench->clauses[i] );
            std::sort( clauses.begin( ) + static_cast<ssize_t>( seg.begin ),
                       clauses.begin( ) + static_cast<ssize_t>( seg.end ) );
        }

        std::vector<Segment> segments( bench->segments.size( ) );
        ::memcpy( segments.data( ), bench->segments.data( ),
                  segments.size( ) * sizeof( Segment ) );

        solver.emplace( /*num_vars=*/bench->info.num_vars, &clauses,
                        &segments );
    }

    auto onSearchOneSolution( ) -> eval::Solution override
    {
        auto res = solver->searchOneSolution( );
        if ( !res ) return { /*solution_opt=*/std::nullopt };

        std::vector<eval::Literal> lits;
        lits.reserve( res->size( ) );
        for ( auto lit : *res ) lits.push_back( toEvalLiteral( lit ) );
        return { /*solution_opt=*/std::move( lits ) };
    }

  private:
    std::optional<BacktrackSolver> solver;
};

}  // namespace

int
main( )
{
    BacktrackDelegator d;
    eval::runAll( &d );
    return 0;
}
