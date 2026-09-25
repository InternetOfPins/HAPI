// expect: [od-rule4] ':' is only valid in a base clause
struct B {int f() const {return 0;}};
struct A {int f() const {return 1+super::f();}};
using X = A:B;
