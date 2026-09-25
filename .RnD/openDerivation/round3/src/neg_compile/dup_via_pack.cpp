// expect: duplicate layer in Z
// Distinct (instantiation time): a duplicate that only exists once the pack is known; the text check cannot see OO
#include <hapi/rules.h>
struct T {int f() const {return 0;}};
struct A {int f() const {return 1+super::f();}};
template<typename... OO> struct Z : (OO : ... : T) {};
int main() {return Z<A,A>{}.f();}
