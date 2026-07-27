// vim: ft=cpp
// forge:v1
//
// See README.md for data structure.
#pragma once

#include <vector>

#include "base.h"

namespace taocp {

struct DCellTable {
      private:
        std::vector<i64> SET;
        std::vector<i64> ITEM;
        std::vector<i64> NODE;

      public:
        DCellTable( size_t item_count )
            : SET( ), ITEM( item_count ), NODE( ) {};

        // Dry run?
};
}  // namespace taocp
