// a type named final that is an OPEN class (it names `super`): a layer, a component's open end, all by its plain name
#include "common.h"
#include <hapi/hapi.h>
struct T {int f() const {return 0;}};
struct A {int f() const {return 1+super::f();}};
struct final {int f() const {return 100+super::f();}};       // an open class named final
struct Z : A:final:final T {};                               // layers A, final; closed on T
struct W : A:final {};                                       // a component whose open end is the class named final
struct Y : W:final T {};
int main() {
  CHECK((Z{}.f()==101 && Y{}.f()==101));
  static_assert(std::is_base_of<hapi::APIOf<T,A,final>,Z>::value && std::is_base_of<hapi::Chain<A,final>,W>::value, "final as a layer");
  DONE("final_layer");
}
