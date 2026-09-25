// expect: cannot bind|invalid initialization|non-const lvalue reference
// rule 2: A:B does not convert to A&
#include <hapi/chain.h>
struct B {};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base;int a=0;  };};
using AB = hapi::Chain<A>::Part<B>;
int main() {AB ab; A& r = ab; (void)r; return 0;}
