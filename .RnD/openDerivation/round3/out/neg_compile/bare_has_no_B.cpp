// expect: has no member named|no member named
// rule 2: (A:B).has_B_method() compiles, A{}.has_B_method() does not
#include <hapi/chain.h>
struct B {void has_B_method() {}};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; };};
using AB = hapi::Chain<A>::Part<B>;
int main() {AB{}.has_B_method(); A{}.has_B_method(); return 0;}
