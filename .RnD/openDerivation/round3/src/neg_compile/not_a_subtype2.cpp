// expect: error
// rule 2: A:B does not convert to A&
#include <hapi/hapi.h>
struct B {};
struct A {int a=0; using super::super;};
struct AB : A:final B {};
int main() {AB ab; A& r = ab; (void)r; return 0;}
