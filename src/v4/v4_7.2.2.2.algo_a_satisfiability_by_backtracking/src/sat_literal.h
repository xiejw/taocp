// vim: ft=cpp
// forge:skip
#ifndef TAOCP_SRC_SAT_LITERAL_H_
#define TAOCP_SRC_SAT_LITERAL_H_

#include <vector>

/* === --- Defined type for literals --------------------------------------- ===
 *
 * Literals follow the encoding on Page 186 of Vol 4B: variable x_k is
 * represented by 2k and its complement ¬x_k by 2k+1. Hence the variable of
 * literal l is l >> 1, and its complement is l ^ 1.
 *
 * === --- How to provide literal
 *
 * - To emit variable x, use 'literal(x)', i.e., 2x;
 * - To emit complement of variable x, use 'complement(literal(x))', i.e.,
 *   2x+1.
 * - Variables are 1-based, so valid literals start from 2.
 *
 * === --- Free Functions
 *
 * - literal returns the (positive) literal 2x of variable x.
 * - complement returns the complement of the literal, i.e., l ^ 1.
 * - decodeVariable returns the variable x of the literal.
 * - isLiteralComplement returns true if the literal is a complement.
 * - printClause prints the clause with literals nicely.
 */
namespace taocp {

using Literal = size_t;
#define PRI_LITERAL "zu"  // Used for printf.

auto literal( size_t var ) -> Literal;
auto complement( Literal lit ) -> Literal;

auto decodeVariable( Literal lit ) -> size_t;
auto isLiteralComplement( Literal lit ) -> bool;
auto printClause( size_t size, const Literal *lits ) -> void;
}  // namespace taocp

#endif  // TAOCP_SRC_SAT_LITERAL_H_
