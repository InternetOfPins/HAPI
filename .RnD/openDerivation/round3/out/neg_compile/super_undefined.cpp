// expect: error
// rule 7: forming A:X where X lacks what A's body asks of `super`, then using it
#include <hapi/chain.h>
struct Empty {};
struct Twice {template<typename O> struct Part:O {
 using Base=O; using Base::Base; static int f(int x) {return 2*Base::f(x);}};};
struct Bad : hapi::Chain<Twice>::Part<Empty> {using Base=hapi::Chain<Twice>::Part<Empty>; using Base::Base;};
int main() {return Bad::f(21);}
