// rule 3: structural identity. Parenthesized right grouping is flattened in both modes; an alias as the right operand
// (Any:X with X = A:B:C) is the same type as Any:A:B:C only in --lower=nested.
#include "identity.h"
using G  = A:(B:C);
using Y  = Any:X;
using Y2 = Any:A:B:C;
template<typename... PP> using F = (PP : ... : C);
static_assert(std::is_same<G,X>::value, "A:(B:C) == A:B:C");
static_assert(std::is_same<F<A,B>,X>::value, "(PP : ... : C) with PP=A,B == A:B:C");
IF_NESTED(static_assert(std::is_same<Y,Y2>::value, "nested: Any:X == Any:A:B:C");)
IF_CHAIN(static_assert(!std::is_same<Y,Y2>::value, "chain: Any:X is Chain<Any>::Part<Chain<A,B>::Part<C>>, not Chain<Any,A,B>::Part<C>");)
int main() { Y y; Y2 y2; CHECK(y.any()==31 && y2.any()==31); DONE("identity"); }
