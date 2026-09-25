// expect: [od-component] 'W' is a component
// members of a component would not be part of any composed object
struct A {int f() const {return super::f();}}; struct B {int f() const {return super::f();}};
struct W : A:B {int extra() const {return 1;}};
