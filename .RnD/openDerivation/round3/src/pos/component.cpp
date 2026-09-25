// a chain with no `final` stays open: a component (hapi::Chain<...>), reusable as a layer and closed where it is used, on a
// terminal the user picks (here the user's own Nil). APIOf starts the collapse. B's unused super-call is fine (rule 7: dependent).
#include "common.h"
#include <hapi/hapi.h>
struct Nil {};
struct T {int h() const {return 0;}};
struct B {int h() const {return 5;} int g() const {return super::missing();}};
struct A {int f() const {return 1+super::h();}};
struct C {int f() const {return 100+super::f();}};
struct W  : A:B {};                                        // component
struct W2 : C:W {};                                        // a component of a component
template<typename... PP> struct WF : (PP : ... : B) {};    // a component fold
struct Z  : W:final Nil {};                                // closed on the user's Nil
struct Z2 : W2:final Nil {int twice() const {return 2*super::f();}};
struct Z3 : W:final T {};                                  // the same component, closed on another terminal
struct Z4 : WF<A>:final Nil {};
struct Z5 : final T {};                                    // closed on T with no layers
int main() {
  CHECK((Z{}.f()==6 && Z2{}.twice()==212 && Z3{}.f()==6 && Z4{}.f()==6 && Z5{}.h()==0));
  static_assert(std::is_base_of<hapi::APIOf<Nil,W>,Z>::value && std::is_base_of<hapi::Chain<A,B>,W>::value, "APIOf closes, Chain stays open");
  static_assert(std::is_base_of<Nil,Z>::value && std::is_base_of<T,Z3>::value, "the terminal is the one the user wrote");
  DONE("component");
}
