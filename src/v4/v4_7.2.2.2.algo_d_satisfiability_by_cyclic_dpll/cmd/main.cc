// forge:skip
#include <stddef.h>
#include <string.h>

#include <optional>
#include <utility>
#include <vector>

#include "sat/eval_runner.h"

#include "sat_dpll.h"
#include "sat_literal.h"

using taocp::complement;
using taocp::decodeVariable;
using taocp::DpllSolver;
using taocp::isLiteralComplement;
using taocp::literal;
using taocp::Literal;
using taocp::Segment;

namespace eval = taocp::sat::eval;

/* `Segment` and `eval::Segment` are separate types, but with identical layout,
 * so the benchmark segments can be copied into the solver's with one memcpy.
 */
TAOCP_EVAL_ASSERT_SEGMENT_LAYOUT( Segment );

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

struct DpllDelegator : eval::Delegator {
    auto name( ) const -> const char * override
    {
        return "Algorithm D: Satisfiability by cyclic DPLL";
    }

    auto onInit( const eval::Bench *bench ) -> void override
    {
        /* Literals are translated at the same indices, so the memcpy'd
         * segments below stay valid. */
        std::vector<Literal> clauses( bench->clauses.size( ) );
        for ( size_t i = 0; i < clauses.size( ); i++ )
            clauses[i] = toSolverLiteral( bench->clauses[i] );

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
    std::optional<DpllSolver> solver;
};

}  // namespace

int
main( )
{
    DpllDelegator d;
    eval::runAll( &d );
    return 0;
}
