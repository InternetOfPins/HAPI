// `final` is only a contextual keyword, so a type named final is legal C++ (struct final {};). In an operand, `final` is the
// keyword only when a type follows it: `final final` closes on the type named final; `final` alone, `final::X` and
// `final<...>` name the type. Here the type named final is a closed terminal API.
#include "common.h"
#include <hapi/hapi.h>
struct final {int f() const {return 7;}};                    // a type named final
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 1+Base::f();}};};
struct B {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 10+Base::f();}};};
struct W  : hapi::Chain<A,B> {static_assert(hapi::Distinct<hapi::Chain<A,B>>, "duplicate layer in W");};                                          // a component
struct Z1 : hapi::APIOf<final,A> {using Base=hapi::APIOf<final,A>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,final>>, "duplicate layer in Z1");}; using Z1_APIOf=hapi::APIOf<final,A>;  namespace hapi { template<> struct Expand< ::Z1> : Expand< ::Z1_APIOf> {}; template<> struct HasOwnRules< ::Z1> : HasOwnRules< ::Z1_APIOf> {}; }                                // closed on the type named final
struct Z2 : hapi::APIOf<final> {using Base=hapi::APIOf<final>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<final>>, "duplicate layer in Z2");}; using Z2_APIOf=hapi::APIOf<final>;  namespace hapi { template<> struct Expand< ::Z2> : Expand< ::Z2_APIOf> {}; template<> struct HasOwnRules< ::Z2> : HasOwnRules< ::Z2_APIOf> {}; }                                  // closed on it, no layers
struct Z3 : hapi::APIOf<final,W> {using Base=hapi::APIOf<final,W>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<W,final>>, "duplicate layer in Z3");}; using Z3_APIOf=hapi::APIOf<final,W>;  namespace hapi { template<> struct Expand< ::Z3> : Expand< ::Z3_APIOf> {}; template<> struct HasOwnRules< ::Z3> : HasOwnRules< ::Z3_APIOf> {}; }                                // the component, closed on it
template<typename... OO> struct Z4 : hapi::APIOf<final,OO...,B> {using Base=hapi::APIOf<final,OO...,B>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<OO...,B,final>>, "duplicate layer in Z4");}; template<typename... OO> using Z4_APIOf=hapi::APIOf<final,OO...,B>;  namespace hapi { template<typename... OO> struct Expand< ::Z4<OO...>> : Expand< ::Z4_APIOf<OO...>> {}; template<typename... OO> struct HasOwnRules< ::Z4<OO...>> : HasOwnRules< ::Z4_APIOf<OO...>> {}; }
int main() {
  CHECK((Z1{}.f()==8 && Z2{}.f()==7 && Z3{}.f()==18 && Z4<A>{}.f()==18));
  static_assert(std::is_base_of<hapi::APIOf<final,A>,Z1>::value && std::is_base_of<hapi::APIOf<final>,Z2>::value, "APIOf<final,...>");
  DONE("final_type");
}
