// a type named final that is an OPEN class (it names `super`): a layer, a component's open end, all by its plain name
#include "common.h"
#include <hapi/hapi.h>
struct T {int f() const {return 0;}};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 1+Base::f();}};};
struct final {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 100+Base::f();}};};       // an open class named final
struct Z : hapi::APIOf<T,A,final> {using Base=hapi::APIOf<T,A,final>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,final,T>>, "duplicate layer in Z");}; using Z_APIOf=hapi::APIOf<T,A,final>;  namespace hapi { template<> struct Expand< ::Z> : Expand< ::Z_APIOf> {}; template<> struct HasOwnRules< ::Z> : HasOwnRules< ::Z_APIOf> {}; }                               // layers A, final; closed on T
struct W : hapi::Chain<A,final> {static_assert(hapi::Distinct<hapi::Chain<A,final>>, "duplicate layer in W");};                                       // a component whose open end is the class named final
struct Y : hapi::APIOf<T,W> {using Base=hapi::APIOf<T,W>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<W,T>>, "duplicate layer in Y");}; using Y_APIOf=hapi::APIOf<T,W>;  namespace hapi { template<> struct Expand< ::Y> : Expand< ::Y_APIOf> {}; template<> struct HasOwnRules< ::Y> : HasOwnRules< ::Y_APIOf> {}; }
int main() {
  CHECK((Z{}.f()==101 && Y{}.f()==101));
  static_assert(std::is_base_of<hapi::APIOf<T,A,final>,Z>::value && std::is_base_of<hapi::Chain<A,final>,W>::value, "final as a layer");
  DONE("final_layer");
}
