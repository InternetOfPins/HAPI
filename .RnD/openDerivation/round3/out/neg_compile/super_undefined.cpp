// expect: error
// rule 7: forming A:X where X lacks what A's body asks of `super`, then using it
#include <hapi/rules.h>
struct Empty {};
struct Twice {template<typename O> struct Part:O {
 using Base=O; using Base::Base; static int f(int x) {return 2*Base::f(x);}};};
struct Bad : hapi::Chain<Twice>::Part<Empty> {using Base=hapi::Chain<Twice>::Part<Empty>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Twice,Empty>>, "duplicate layer in Bad");};
int main() {return Bad::f(21);}
