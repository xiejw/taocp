// vim: ft=cpp
// forge:v1
#ifndef TAOCP_SRC_SAT_LITERAL_H_
#define TAOCP_SRC_SAT_LITERAL_H_

#include <vector>

/* === --- Defined type for literals --------------------------------------- ===
 *
 * Any caller should use encoded literal when emitting clauses. This helps to
 * wrap both the variable and its complement (see Page 186 of Vol 4B for
 * literal definition).
 *
 * Internal representation is opaque to callers.
 *
 * === --- How to provide literal
 *
 * - To emit variable x, use 'x';
 * - To emit complement of variable x, use 'complement(x)'.
 * - Both cases are 1-based literals.
 *
 * === --- Free Functions
 *
 * - decodeVariable returns the variable x of the literal.
 * - isLiteralComplement returns true if the encoded value is a complement.
 * - printClause prints the clause with literals nicely.
 */
namespace taocp {

using Literal = size_t;
#define PRI_LITERAL "zu"  // Used for printf.

auto complement( Literal lit ) -> Literal;

auto decodeVariable( Literal lit ) -> Literal;
auto isLiteralComplement( Literal lit ) -> bool;
auto printClause( size_t size, const Literal *lits ) -> void;
}  // namespace taocp

#endif  // TAOCP_SRC_SAT_LITERAL_H_
