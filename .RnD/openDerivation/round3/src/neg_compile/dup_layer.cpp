// expect: duplicate layer in Y
// A over a component X that already holds A: rejected by the compiler (hapi::Distinct splices X's Types)
#include <hapi/hapi.h>
struct A {int f() const {return 1+super::f();}};
struct B {int f() const {return 2+super::f();}};
struct C {int f() const {return 0;}};
struct X : A:B {};
struct Y : A:X:final C {};
