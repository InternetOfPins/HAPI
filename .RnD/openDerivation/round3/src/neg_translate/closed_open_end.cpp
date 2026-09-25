// expect: [od-final] 'K' is closed
// a closed class as the open end of a component: close on it instead (struct Z : A:final K {})
struct K {int f() const {return 0;}}; struct A {int f() const {return 1+super::f();}};
struct Z : A:K {};
