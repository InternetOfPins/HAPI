// `final` is only a contextual keyword, so a type named final is legal C++ (struct final {};). In an operand, `final` is the
// keyword only when a type follows it: `final final` closes on the type named final; `final` alone, `final::X` and
// `final<...>` name the type. Here the type named final is a closed terminal API.
#include "common.h"
#include <hapi/hapi.h>
struct final {int f() const {return 7;}};                    // a type named final
struct A {int f() const {return 1+super::f();}};
struct B {int f() const {return 10+super::f();}};
struct W  : A:B {};                                          // a component
struct Z1 : A:final final {};                                // closed on the type named final
struct Z2 : final final {};                                  // closed on it, no layers
struct Z3 : W:final final {};                                // the component, closed on it
template<typename... OO> struct Z4 : (OO : ... : B : final final) {};
int main() {
  CHECK((Z1{}.f()==8 && Z2{}.f()==7 && Z3{}.f()==18 && Z4<A>{}.f()==18));
  static_assert(std::is_base_of<hapi::APIOf<final,A>,Z1>::value && std::is_base_of<hapi::APIOf<final>,Z2>::value, "APIOf<final,...>");
  DONE("final_type");
}
