// expect: error
// rule 7: forming A:X where X lacks what A's body asks of `super`, then using it
#include <hapi/hapi.h>
struct Empty {};
struct Twice {template<typename O> struct Part:O {
 using Base=O; using Base::Base; static int f(int x) {return 2*Base::f(x);}};};
struct Bad : hapi::APIOf<Empty,Twice> {using Base=hapi::APIOf<Empty,Twice>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Twice,Empty>>, "duplicate layer in Bad");}; using Bad_APIOf=hapi::APIOf<Empty,Twice>;  namespace hapi { template<> struct Expand<Bad> : Expand<Bad_APIOf> {}; template<> struct HasOwnRules<Bad> : HasOwnRules<Bad_APIOf> {}; }
int main() {return Bad::f(21);}
