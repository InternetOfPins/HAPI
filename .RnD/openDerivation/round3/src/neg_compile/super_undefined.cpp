// expect: error
// rule 7: forming A:X where X lacks what A's body asks of `super`, then using it
#include <hapi/hapi.h>
struct Empty {};
struct Twice {static int f(int x) {return 2*super::f(x);}};
struct Bad : Twice:final Empty {};
int main() {return Bad::f(21);}
