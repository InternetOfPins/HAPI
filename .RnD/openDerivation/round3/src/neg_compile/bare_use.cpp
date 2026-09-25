// expect: is not a member of|no member named
// rule 7: a class that names `super` is only usable as A:X; bare use is a lookup error
#include <hapi/chain.h>
struct Twice {static int f(int x) {return 2*super::f(x);}};
int main() {return Twice::f(21);}
