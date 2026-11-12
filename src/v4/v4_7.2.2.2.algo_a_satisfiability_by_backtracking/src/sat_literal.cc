// forge:v1
#include "sat_literal.h"

#include <stdio.h>

namespace taocp {
namespace {
constexpr Literal kMask = Literal{ 1 } << ( 8 * sizeof( Literal ) - 1 );
}

auto
complement( Literal lit ) -> Literal
{
    return lit | kMask;
}

auto
decodeVariable( Literal lit ) -> Literal
{
    return lit & ( ~kMask );
}

auto
isLiteralComplement( Literal lit ) -> bool
{
    return lit & kMask;
}

auto
printClause( size_t size, const Literal *lits ) -> void
{
    if ( size == 0 ) {
        printf( "(empty literals)\n" );
        return;
    }

    printf( "< " );
    for ( size_t x = 0; x < size; x++ ) {
        auto i = lits[x];
        if ( isLiteralComplement( i ) )
            printf( "C(%3" PRI_LITERAL "), ", decodeVariable( i ) );
        else
            printf( "%3" PRI_LITERAL ", ", i );
    }
    printf( " >\n" );
}

}  // namespace taocp
