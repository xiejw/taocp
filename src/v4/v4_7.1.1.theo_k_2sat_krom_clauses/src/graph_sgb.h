// vim: ft=cpp
// forge:v2
//
#include <vector>

#include <assert.h>

namespace taocp {
struct SGBNode {
        // === --- User Inputs --------------------------------------------- ===

      public:  // Initialized by users.
        std::vector<struct SGBNode *> arcs;

      public:
        SGBNode( ) = default;

        // === --- Internal Data Structures -------------------------------- ===

      public:  // Used by algorithm. No need to initialize.
        using ArcIter = std::vector<struct SGBNode *>::iterator;

        struct SGBNode *parent;  // Parent node.
        ArcIter         arc;     // Store the state of next arc.

        // Dual usage. A) Stack link B) See Vol F12A Page 10-(11)
        struct SGBNode *link;

        // Dual usage: A) LOW B) See Vol F12A Page 10-(12)
        size_t rep;
};

struct SGBGraph {
        // === --- Internal Data Structures -------------------------------- ===

      private:
        std::vector<SGBNode> vertices_;
#ifndef NDEBUG
        size_t num_vertices_expected_;
#endif

      private:
        // See runAlgoT.
        std::vector<size_t> component_ids_;

        // === --- User Inputs --------------------------------------------- ===
      public:
        // Ensure the points are stable.  So vertices_ is allocated.
        SGBGraph( size_t num_vertices )
            : vertices_( num_vertices )
#ifndef NDEBUG
              ,
              num_vertices_expected_( num_vertices )
#endif
        {
        }

        // === --- Public APIs --------------------------------------------- ===
      public:
        /// Return the SGBNode at position i.
        SGBNode *getVertex( size_t i )
        {
                assert( i < num_vertices_expected_ );
                return &this->vertices_[i];
        }

        /// Run Algorithm T (V4F12A Strong Components) and then
        /// getComponentIdsAfterAlgoT can be used.
        void runAlgoT( );

        /// Return the component ids corresponding to the vertices, one id for
        /// each vertex. The value domain for id is not defined. But it is
        /// guaranteed that
        ///
        ///     0<= id < vertices.size();
        const std::vector<size_t> *getComponentIdsAfterAlgoT( ) const
        {
                return &component_ids_;
        };
};
}  // namespace taocp
