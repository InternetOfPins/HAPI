// expect: [od-rule8] 'X' is a closed composition
// a closed composition cannot be a layer: compose the component instead
struct T {int f() const {return 0;}}; struct A {int f() const {return super::f();}}; struct B {int f() const {return super::f();}};
struct X : A:final T {};
struct Y : B:X:final T {};
