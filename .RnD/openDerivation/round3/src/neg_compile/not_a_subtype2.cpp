// expect: cannot bind|invalid initialization|non-const lvalue reference
// rule 2: A:B does not convert to A&
#include <hapi/chain.h>
struct B {};
struct A {int a=0; using super::super;};
using AB = A:B;
int main() {AB ab; A& r = ab; (void)r; return 0;}
