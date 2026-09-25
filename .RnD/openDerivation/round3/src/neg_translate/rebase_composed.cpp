// expect: [od-rule8] a composed class as a left operand
struct A {int f() const {return super::f();}};
struct B {int f() const {return super::f();}};
struct C {int f() const {return 0;}};
struct Z : (A:B):C {};
