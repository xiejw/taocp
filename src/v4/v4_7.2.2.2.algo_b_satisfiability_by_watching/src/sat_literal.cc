// forge:skip
#include "sat_literal.h"

#include <stdio.h>

namespace taocp {

auto
literal( size_t var ) -> Literal
{
    return 2 * var;
}

auto
complement( Literal lit ) -> Literal
{
    return lit ^ 1;
}

auto
decodeVariable( Literal lit ) -> size_t
{
    return lit >> 1;
}

auto
isLiteralComplement( Literal lit ) -> bool
{
    return lit & 1;
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
            printf( "¬%3zu, ", decodeVariable( i ) );
        else
            printf( "%3zu, ", decodeVariable( i ) );
    }
    printf( " >\n" );
}

}  // namespace taocp
