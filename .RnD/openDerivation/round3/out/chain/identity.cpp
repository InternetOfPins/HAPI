// rule 3: structural identity. Parenthesized right grouping is flattened in both modes; an alias as the right operand
// (Any:X with X = A:B:C) is the same type as Any:A:B:C only in --lower=nested.
#include "identity.h"
using G  = hapi::Chain<A,B>::Part<C>;
using Y  = hapi::Chain<Any>::Part<X>;
using Y2 = hapi::Chain<Any,A,B>::Part<C>;
template<typename... PP> using F = typename hapi::Chain<PP...>::template Part<C>;
static_assert(std::is_same<G,X>::value, "A:(B:C) == A:B:C");
static_assert(std::is_same<F<A,B>,X>::value, "(PP : ... : C) with PP=A,B == A:B:C");
IF_NESTED(static_assert(std::is_same<Y,Y2>::value, "nested: Any:X == Any:A:B:C");)
IF_CHAIN(static_assert(!std::is_same<Y,Y2>::value, "chain: Any:X is Chain<Any>::Part<Chain<A,B>::Part<C>>, not Chain<Any,A,B>::Part<C>");)
int main() { Y y; Y2 y2; CHECK(y.any()==31 && y2.any()==31); DONE("identity"); }
