// forge:skip
#include <string.h>

#include "log.h"
#include "sat_cdcl.h"
#include "sat_literal.h"

using taocp::C;
using taocp::CDCLSolver;
using taocp::literal_t;
using taocp::PrintClause;

namespace {

auto
RunSolver( ) -> void
{
        INFO( "==== --- Demo Problem for Algorithm C --- ===" );
        INFO( "==== --- Satisfiability by CDCL --------- ===" );
}
}  // namespace

int
main( )
{
        RunSolver( );
        return 0;
}
