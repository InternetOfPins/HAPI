// expect: B only after A
// B with no A before it: a component's rules<Before,After>() rejects the composition (HAPI's rule walk in every composed struct)
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
struct Z : hapi::APIOf<T,B> {using Base=hapi::APIOf<T,B>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<B,T>>, "duplicate layer in Z");}; using Z_APIOf=hapi::APIOf<T,B>;  namespace hapi { template<> struct Expand< ::Z> : Expand< ::Z_APIOf> {}; template<> struct HasOwnRules< ::Z> : HasOwnRules< ::Z_APIOf> {}; }
