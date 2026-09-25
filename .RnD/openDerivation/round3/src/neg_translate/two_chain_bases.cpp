// expect: [od-rule6] 'Z' has 2 ':' bases
struct A {int f() const {return super::f();}};
struct B {int g() const {return super::g();}};
struct T {int f() const {return 0;}}; struct U {int g() const {return 0;}};
struct Z : A:final T, B:final U {};
