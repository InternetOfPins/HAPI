// a chain with no `final` stays open: a component (hapi::Chain<...>), reusable as a layer and closed where it is used, on a
// terminal the user picks (here the user's own Nil). APIOf starts the collapse. B's unused super-call is fine (rule 7: dependent).
#include "common.h"
#include <hapi/hapi.h>
struct Nil {};
struct T {int h() const {return 0;}};
struct B {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int h() const {return 5;} int g() const {return Base::missing();}};};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 1+Base::h();}};};
struct C {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 100+Base::f();}};};
struct W  : hapi::Chain<A,B> {static_assert(hapi::Distinct<hapi::Chain<A,B>>, "duplicate layer in W");};                                        // component
struct W2 : hapi::Chain<C,W> {static_assert(hapi::Distinct<hapi::Chain<C,W>>, "duplicate layer in W2");};                                        // a component of a component
template<typename... PP> struct WF : hapi::Chain<PP...,B> {static_assert(hapi::Distinct<hapi::Chain<PP...,B>>, "duplicate layer in WF");};    // a component fold
struct Z  : hapi::APIOf<Nil,W> {using Base=hapi::APIOf<Nil,W>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<W,Nil>>, "duplicate layer in Z");};                                // closed on the user's Nil
struct Z2 : hapi::APIOf<Nil,W2> {using Base=hapi::APIOf<Nil,W2>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<W2,Nil>>, "duplicate layer in Z2");int twice() const {return 2*Base::f();}};
struct Z3 : hapi::APIOf<T,W> {using Base=hapi::APIOf<T,W>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<W,T>>, "duplicate layer in Z3");};                                  // the same component, closed on another terminal
struct Z4 : hapi::APIOf<Nil,WF<A>> {using Base=hapi::APIOf<Nil,WF<A>>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<WF<A>,Nil>>, "duplicate layer in Z4");};
struct Z5 : hapi::APIOf<T> {using Base=hapi::APIOf<T>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<T>>, "duplicate layer in Z5");};                                    // closed on T with no layers
int main() {
  CHECK((Z{}.f()==6 && Z2{}.twice()==212 && Z3{}.f()==6 && Z4{}.f()==6 && Z5{}.h()==0));
  static_assert(std::is_base_of<hapi::APIOf<Nil,W>,Z>::value && std::is_base_of<hapi::Chain<A,B>,W>::value, "APIOf closes, Chain stays open");
  static_assert(std::is_base_of<Nil,Z>::value && std::is_base_of<T,Z3>::value, "the terminal is the one the user wrote");
  DONE("component");
}
