// expect: B only after A|A must be before B
// A after B, only known once the pack is: a component's rules<Before,After>() rejects the composition (HAPI's rule walk in every composed struct)
#include <hapi/hapi.h>
struct T {int f() const {return 0;}};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 1+Base::f();}};};
struct B {template<typename O> struct Part:O {                                        // fx.h's rules, in ':' form: kept on the holder by the translator
  using Base=O; using Base::Base;
  
  int f() const {return 10+Base::f();}
}; template<typename Before,typename After> static constexpr bool rules() {
    static_assert(hapi::query<hapi::SameAs<A>,Before>,"B only after A");
    static_assert(!hapi::query<hapi::SameAs<A>,After>,"A must be before B");
    return true;
  }};
template<typename... OO> struct ZF : hapi::APIOf<T,OO...> {using Base=hapi::APIOf<T,OO...>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<OO...,T>>, "duplicate layer in ZF");}; template<typename... OO> using ZF_APIOf=hapi::APIOf<T,OO...>;  namespace hapi { template<typename... OO> struct Expand<ZF<OO...>> : Expand<ZF_APIOf<OO...>> {}; template<typename... OO> struct HasOwnRules<ZF<OO...>> : HasOwnRules<ZF_APIOf<OO...>> {}; }
int main() {return ZF<B,A>{}.f();}
