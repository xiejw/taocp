// forge:skip
#include <optional>
#include <utility>
#include <vector>

#include "sat/eval_runner.h"

#include "sat_literal.h"
#include "sat_backtrack.h"

using taocp::complement;
using taocp::decodeVariable;
using taocp::isLiteralComplement;
using taocp::literal;
using taocp::Literal;
using taocp::BacktrackSolver;

namespace eval = taocp::sat::eval;

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
        solver.emplace( /*num_vars=*/bench->info.num_vars,
                        /*num_clauses=*/bench->segments.size( ) );
        std::vector<Literal> lits;
        for ( auto &seg : bench->segments ) {
            lits.clear( );
            for ( size_t i = seg.begin; i < seg.end; i++ )
                lits.push_back( toSolverLiteral( bench->clauses[i] ) );
            solver->emitClause( lits.size( ), lits.data( ) );
        }
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
