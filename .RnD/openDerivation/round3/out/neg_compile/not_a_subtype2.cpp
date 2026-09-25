// expect: error
// rule 2: A:B does not convert to A&
#include <hapi/rules.h>
struct B {};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int a=0;  };};
struct AB : hapi::Chain<A>::Part<B> {using Base=hapi::Chain<A>::Part<B>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,B>>, "duplicate layer in AB");};
int main() {AB ab; A& r = ab; (void)r; return 0;}
