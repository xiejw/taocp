// forge:skip
#include <string.h>

#include "log.h"
#include "sat_cdcl.h"
#include "sat_literal.h"

using taocp::C;
using taocp::CDCLSolver;
using taocp::literal_t;
using taocp::printClause;

namespace {

auto
runSolver( ) -> void
{
        INFO( "==== --- Demo Problem for Algorithm C --- ===" );
        INFO( "==== --- Satisfiability by CDCL --------- ===" );

        // Emit clause
}
}  // namespace

int
main( )
{
        runSolver( );
        return 0;
}
