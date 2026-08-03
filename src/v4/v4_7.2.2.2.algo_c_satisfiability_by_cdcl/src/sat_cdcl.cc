// forge:v1
#include "sat_cdcl.h"

#include <stdio.h>

#include "log.h"

#define DEBUG_MODE 0

namespace taocp {

auto
CDCLSolver::searchOneSolution( ) -> std::optional<std::vector<literal_t>>
{
        return std::nullopt;
}

}  // namespace taocp
