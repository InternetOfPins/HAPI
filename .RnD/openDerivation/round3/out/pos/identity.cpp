// rule 3, struct-only form: a composition is a named struct, so identity is its name (nominal), in every TU
// (identity_tu1/2). Two structs over the same chain are different types with the same base and the same behaviour.
#include "identity.h"
struct G  : hapi::APIOf<C,A,B> {using Base=hapi::APIOf<C,A,B>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,B,C>>, "duplicate layer in G");}; using G_APIOf=hapi::APIOf<C,A,B>;  namespace hapi { template<> struct Expand<G> : Expand<G_APIOf> {}; template<> struct HasOwnRules<G> : HasOwnRules<G_APIOf> {}; }                                  // parenthesized right grouping: flattened
struct Y  : hapi::APIOf<X,Any> {using Base=hapi::APIOf<X,Any>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Any,X>>, "duplicate layer in Y");}; using Y_APIOf=hapi::APIOf<X,Any>;  namespace hapi { template<> struct Expand<Y> : Expand<Y_APIOf> {}; template<> struct HasOwnRules<Y> : HasOwnRules<Y_APIOf> {}; }                                    // X (a named composition) as the last operand
struct Y2 : hapi::APIOf<C,Any,A,B> {using Base=hapi::APIOf<C,Any,A,B>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Any,A,B,C>>, "duplicate layer in Y2");}; using Y2_APIOf=hapi::APIOf<C,Any,A,B>;  namespace hapi { template<> struct Expand<Y2> : Expand<Y2_APIOf> {}; template<> struct HasOwnRules<Y2> : HasOwnRules<Y2_APIOf> {}; }
template<typename... PP> struct F : hapi::APIOf<C,PP...> {using Base=hapi::APIOf<C,PP...>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<PP...,C>>, "duplicate layer in F");}; template<typename... PP> using F_APIOf=hapi::APIOf<C,PP...>;  namespace hapi { template<typename... PP> struct Expand<F<PP...>> : Expand<F_APIOf<PP...>> {}; template<typename... PP> struct HasOwnRules<F<PP...>> : HasOwnRules<F_APIOf<PP...>> {}; }
static_assert(!std::is_same<G,X>::value && std::is_same<G::Base,X::Base>::value, "A:(B:C) and A:B:C: different names, same base");
static_assert(std::is_same<F<A,B>::Base,X::Base>::value, "(PP : ... : C) with PP=A,B: the same base as A:B:C");
static_assert(!std::is_same<Y::Base,Y2::Base>::value && std::is_base_of<X,Y>::value && !std::is_base_of<X,Y2>::value,
              "Any:X ends at the struct X; Any:A:B:C is one flat chain: different bases");
int main() { Y y; Y2 y2; CHECK(y.any()==31 && y2.any()==31); DONE("identity"); }
