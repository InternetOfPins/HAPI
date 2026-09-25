// expect: error
// rule 2: A:B does not convert to A&
#include <hapi/hapi.h>
struct B {};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int a=0;  };};
struct AB : hapi::APIOf<B,A> {using Base=hapi::APIOf<B,A>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,B>>, "duplicate layer in AB");}; using AB_APIOf=hapi::APIOf<B,A>;  namespace hapi { template<> struct Expand< ::AB> : Expand< ::AB_APIOf> {}; template<> struct HasOwnRules< ::AB> : HasOwnRules< ::AB_APIOf> {}; }
int main() {AB ab; A& r = ab; (void)r; return 0;}
