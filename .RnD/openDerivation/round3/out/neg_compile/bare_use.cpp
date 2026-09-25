// expect: error
// rule 7: a class that names `super` is only usable as A:X; bare use is a lookup error
#include <hapi/rules.h>
struct Twice {template<typename O> struct Part:O {
 using Base=O; using Base::Base; static int f(int x) {return 2*Base::f(x);}};};
int main() {return Twice::f(21);}
