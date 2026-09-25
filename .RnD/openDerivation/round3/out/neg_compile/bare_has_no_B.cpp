// expect: error
// rule 2: (A:B).has_B_method() compiles, A{}.has_B_method() does not
#include <hapi/hapi.h>
struct B {void has_B_method() {}};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base;  };};
struct AB : hapi::APIOf<B,A> {using Base=hapi::APIOf<B,A>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,B>>, "duplicate layer in AB");}; using AB_APIOf=hapi::APIOf<B,A>;  namespace hapi { template<> struct Expand< ::AB> : Expand< ::AB_APIOf> {}; template<> struct HasOwnRules< ::AB> : HasOwnRules< ::AB_APIOf> {}; }
int main() {AB{}.has_B_method(); A{}.has_B_method(); return 0;}
