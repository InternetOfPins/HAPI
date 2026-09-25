// expect: is not a member of|no member named
// rule 7: forming A:X where X lacks what A's body asks of `super`, then using it
#include <hapi/chain.h>
struct Empty {};
struct Twice {static int f(int x) {return 2*super::f(x);}};
using Bad = Twice:Empty;
int main() {return Bad::f(21);}
