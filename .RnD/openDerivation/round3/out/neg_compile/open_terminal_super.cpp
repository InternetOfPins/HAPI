// expect: error
// amendment: with an open rightmost operand the chain ends in od::Nil; using B's super-call reaches it and fails
#include <hapi/rules.h>
#include "od_nil.h"
struct B {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int h() const {return 5;} int g() const {return Base::missing();}};};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 1+Base::h();}};};
struct Z : hapi::Chain<A,B>::Part<od::Nil> {using Base=hapi::Chain<A,B>::Part<od::Nil>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,B>>, "duplicate layer in Z");};
int main() {return Z{}.g();}
