// rule 3, struct-only form: a composition is a named struct, so identity is its name (nominal), in every TU
// (identity_tu1/2). Two structs over the same chain are different types with the same base and the same behaviour.
#include "identity.h"
struct G  : hapi::APIOf<C,A,B> {using Base=hapi::APIOf<C,A,B>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,B,C>>, "duplicate layer in G");};                                  // parenthesized right grouping: flattened
struct Y  : hapi::APIOf<X,Any> {using Base=hapi::APIOf<X,Any>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Any,X>>, "duplicate layer in Y");};                                    // X (a named composition) as the last operand
struct Y2 : hapi::APIOf<C,Any,A,B> {using Base=hapi::APIOf<C,Any,A,B>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Any,A,B,C>>, "duplicate layer in Y2");};
template<typename... PP> struct F : hapi::APIOf<C,PP...> {using Base=hapi::APIOf<C,PP...>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<PP...,C>>, "duplicate layer in F");};
static_assert(!std::is_same<G,X>::value && std::is_same<G::Base,X::Base>::value, "A:(B:C) and A:B:C: different names, same base");
static_assert(std::is_same<F<A,B>::Base,X::Base>::value, "(PP : ... : C) with PP=A,B: the same base as A:B:C");
static_assert(!std::is_same<Y::Base,Y2::Base>::value && std::is_base_of<X,Y>::value && !std::is_base_of<X,Y2>::value,
              "Any:X ends at the struct X; Any:A:B:C is one flat chain: different bases");
int main() { Y y; Y2 y2; CHECK(y.any()==31 && y2.any()==31); DONE("identity"); }
