// expect: has no member named|no member named
// rule 2: (A:B).has_B_method() compiles, A{}.has_B_method() does not
#include <hapi/chain.h>
struct B {void has_B_method() {}};
struct A {using super::super;};
using AB = A:B;
int main() {AB{}.has_B_method(); A{}.has_B_method(); return 0;}
