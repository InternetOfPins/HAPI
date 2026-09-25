// expect: error
// amendment: with an open rightmost operand the chain ends in od::Nil; using B's super-call reaches it and fails
#include <hapi/chain.h>
#include "od_nil.h"
struct B {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int h() const {return 5;} int g() const {return Base::missing();}};};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 1+Base::h();}};};
struct Z : hapi::Chain<A,B>::Part<od::Nil> {using Base=hapi::Chain<A,B>::Part<od::Nil>; using Base::Base;};
int main() {return Z{}.g();}
