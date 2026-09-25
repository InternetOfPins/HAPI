// expect: error
// rule 2: (A:B).has_B_method() compiles, A{}.has_B_method() does not
#include <hapi/rules.h>
struct B {void has_B_method() {}};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base;  };};
struct AB : hapi::Chain<A>::Part<B> {using Base=hapi::Chain<A>::Part<B>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,B>>, "duplicate layer in AB"); static_assert(hapi::BuildRules<hapi::Chain<>,hapi::Chain<B,A>>::rules(), "HAPI: validation failed in AB");};
int main() {AB{}.has_B_method(); A{}.has_B_method(); return 0;}
