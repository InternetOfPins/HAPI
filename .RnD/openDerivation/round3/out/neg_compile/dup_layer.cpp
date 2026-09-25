// expect: duplicate layer in Y
// A:X where X already holds A: rejected by the compiler (hapi::Distinct), like `struct X : Nil, Nil {}` is
#include <hapi/rules.h>
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 1+Base::f();}};};
struct C {int f() const {return 0;}};
struct X : hapi::Chain<A>::Part<C> {using Base=hapi::Chain<A>::Part<C>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,C>>, "duplicate layer in X");};
struct Y : hapi::Chain<A>::Part<X> {using Base=hapi::Chain<A>::Part<X>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,X>>, "duplicate layer in Y");};
