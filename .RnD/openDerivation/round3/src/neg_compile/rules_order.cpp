// expect: B only after A|A must be before B
// A after B: a component's rules<Before,After>() rejects the composition (HAPI's rule walk in every composed struct)
#include <hapi/rules.h>
struct T {int f() const {return 0;}};
struct A {int f() const {return 1+super::f();}};
struct B {                                        // fx.h's rules, in ':' form: kept on the holder by the translator
  template<typename Before,typename After> static constexpr bool rules() {
    static_assert(hapi::query<hapi::SameAs<A>,Before>,"B only after A");
    static_assert(!hapi::query<hapi::SameAs<A>,After>,"A must be before B");
    return true;
  }
  int f() const {return 10+super::f();}
};
struct Z : B:A:T {};
