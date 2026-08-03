// forge:v1
#include "sat_literal.h"

#include <stdio.h>

namespace taocp {
namespace {
constexpr size_t mask = 1 << ( sizeof( literal_t ) - 1 );
}

auto
C( literal_t c ) -> literal_t
{
        return c | mask;
}

auto
decodeRawLiteralValue( literal_t c ) -> literal_t
{
        return c & ( ~mask );
}

auto
isLiteralComplement( literal_t c ) -> bool
{
        return c & mask;
}

auto
printClause( size_t size, const literal_t *lits ) -> void
{
        if ( size == 0 ) {
                printf( "(empty literals)\n" );
                return;
        }

        printf( "< " );
        for ( size_t x = 0; x < size; x++ ) {
                auto i = lits[x];
                if ( isLiteralComplement( i ) )
                        printf( "C(%3" PRI_literal "), ",
                                decodeRawLiteralValue( i ) );
                else
                        printf( "%3" PRI_literal ", ", i );
        }
        printf( " >\n" );
}

}  // namespace taocp
