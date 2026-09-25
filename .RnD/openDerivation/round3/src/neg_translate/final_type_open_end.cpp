// expect: [od-final] 'final' is closed (its body never names `super`): close the composition on it with `final final`
// a CLOSED type named final as a component's open end: close on it instead
struct final {int f() const {return 7;}};
struct A {int f() const {return 1+super::f();}};
struct Z : A:final {};
