// expect: [od-final] `final` marks the terminal
struct T {}; struct A {int f() const {return super::f();}};
struct Z : A:final T:A {};
