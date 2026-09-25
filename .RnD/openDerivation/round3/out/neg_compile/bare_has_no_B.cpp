// expect: error
// rule 2: (A:B).has_B_method() compiles, A{}.has_B_method() does not
#include <hapi/hapi.h>
struct B {void has_B_method() {}};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base;  };};
struct AB : hapi::APIOf<B,A> {using Base=hapi::APIOf<B,A>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,B>>, "duplicate layer in AB");};
int main() {AB{}.has_B_method(); A{}.has_B_method(); return 0;}
