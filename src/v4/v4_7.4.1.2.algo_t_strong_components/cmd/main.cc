// forge:skip
#include <stdio.h>

#include "graph_sgb.h"
#include "log.h"
#include "test_macros.h"

namespace {
using namespace taocp;

FORGE_TEST( test_single_node )
{
        SGBGraph g{ 1 };

        g.runAlgoT( );
        auto ids = g.getComponentIdsAfterAlgoT( );

        EXPECT_TRUE( ids->size( ) == 1, "id array size" );
        EXPECT_TRUE( ids->at( 0 ) == 0, "id" );

        return NULL;
}

FORGE_TEST( test_two_isolated_nodes )
{
        SGBGraph g{ 2 };
        {
                auto *v = g.getVertex( 0 );
                auto *u = g.getVertex( 1 );

                v->arcs.push_back( u );
        }

        g.runAlgoT( );
        auto ids = g.getComponentIdsAfterAlgoT( );

        EXPECT_TRUE( ids->size( ) == 2, "id array size" );
        EXPECT_TRUE( ids->at( 0 ) == 0, "id" );
        EXPECT_TRUE( ids->at( 1 ) == 1, "id" );

        return NULL;
}

FORGE_TEST( test_two_connected_nodes )
{
        SGBGraph g{ 2 };
        {
                auto *v = g.getVertex( 0 );
                auto *u = g.getVertex( 1 );

                v->arcs.push_back( u );
                u->arcs.push_back( v );
        }

        g.runAlgoT( );
        auto ids = g.getComponentIdsAfterAlgoT( );

        EXPECT_TRUE( ids->size( ) == 2, "id array size" );
        EXPECT_TRUE( ids->at( 0 ) == ids->at( 1 ), "id" );
        return NULL;
}

FORGE_TEST( test_two_components )
{
        SGBGraph g{ 4 };
        {
                auto *v = g.getVertex( 0 );
                auto *u = g.getVertex( 1 );
                auto *w = g.getVertex( 2 );
                auto *x = g.getVertex( 3 );

                // v <-> u <-> w
                v->arcs.push_back( u );
                u->arcs.push_back( w );
                w->arcs.push_back( v );

                // {v,u,w} -> x
                v->arcs.push_back( x );
                u->arcs.push_back( x );
                w->arcs.push_back( x );
        }

        g.runAlgoT( );
        auto ids = g.getComponentIdsAfterAlgoT( );

        EXPECT_TRUE( ids->size( ) == 4, "id array size" );
        EXPECT_TRUE( ids->at( 0 ) == ids->at( 1 ), "id" );
        EXPECT_TRUE( ids->at( 2 ) == ids->at( 1 ), "id" );
        EXPECT_TRUE( ids->at( 2 ) != ids->at( 3 ), "id" );
        return NULL;
}
}  // namespace

int
main( )
{
        forge::test_suite_run( );
}
