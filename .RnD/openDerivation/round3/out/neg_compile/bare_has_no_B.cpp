// expect: error
// rule 2: (A:B).has_B_method() compiles, A{}.has_B_method() does not
#include <hapi/chain.h>
struct B {void has_B_method() {}};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base;  };};
struct AB : hapi::Chain<A>::Part<B> {using Base=hapi::Chain<A>::Part<B>; using Base::Base;};
int main() {AB{}.has_B_method(); A{}.has_B_method(); return 0;}
