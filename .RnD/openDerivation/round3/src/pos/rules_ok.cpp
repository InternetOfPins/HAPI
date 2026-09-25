// HAPI's rules system in the ':' form: every composed struct runs BuildRules over the list APIOf would validate
// (terminal first, then the layers outer to inner), so a component's rules<Before,After>() is enforced here too
#include "common.h"
#include <hapi/hapi.h>
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
struct Z : A:B:T {};
template<typename... OO> struct ZF : (OO : ... : T) {};
static_assert(hapi::BuildRules<hapi::Chain<>,hapi::Chain<T,A,B>>::rules(), "the same list, as APIOf<T,A,B> validates it");
int main() {
  CHECK((Z{}.f()==11 && ZF<A,B>{}.f()==11 && hapi::APIOf<T,A,B>{}.f()==11));
  DONE("rules_ok");
}
