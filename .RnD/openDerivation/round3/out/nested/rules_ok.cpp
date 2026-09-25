// HAPI's rules system in the ':' form: every composed struct runs BuildRules over the list APIOf would validate
// (terminal first, then the layers outer to inner), so a component's rules<Before,After>() is enforced here too
#include "common.h"
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
struct Z : A::Part<B::Part<T>> {using Base=A::Part<B::Part<T>>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,B,T>>, "duplicate layer in Z"); static_assert(hapi::BuildRules<hapi::Chain<>,hapi::Chain<T,A,B>>::rules(), "HAPI: validation failed in Z");};
template<typename... OO> struct ZF : od::FoldT<T,OO...> {using Base=typename od::FoldT<T,OO...>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<OO...,T>>, "duplicate layer in ZF"); static_assert(hapi::BuildRules<hapi::Chain<>,hapi::Chain<T,OO...>>::rules(), "HAPI: validation failed in ZF");};
static_assert(hapi::BuildRules<hapi::Chain<>,hapi::Chain<T,A,B>>::rules(), "the same list, as APIOf<T,A,B> validates it");
int main() {
  CHECK((Z{}.f()==11 && ZF<A,B>{}.f()==11 && hapi::APIOf<T,A,B>{}.f()==11));
  DONE("rules_ok");
}
