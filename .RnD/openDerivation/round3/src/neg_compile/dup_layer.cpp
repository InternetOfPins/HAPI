// expect: duplicate layer in Y
// A:X where X already holds A: rejected by the compiler (hapi::Distinct), like `struct X : Nil, Nil {}` is
#include <hapi/rules.h>
struct A {int f() const {return 1+super::f();}};
struct C {int f() const {return 0;}};
struct X : A:C {};
struct Y : A:X {};
