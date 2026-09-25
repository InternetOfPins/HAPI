// expect: duplicate layer in Z
// Distinct (instantiation time): a duplicate that only exists once the pack is known; the text check cannot see OO
#include <hapi/rules.h>
struct T {int f() const {return 0;}};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 1+Base::f();}};};
template<typename... OO> struct Z : hapi::Chain<OO...>::template Part<T> {using Base=typename hapi::Chain<OO...>::template Part<T>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<OO...,T>>, "duplicate layer in Z"); static_assert(hapi::BuildRules<hapi::Chain<>,hapi::Chain<T,OO...>>::rules(), "HAPI: validation failed in Z");};
int main() {return Z<A,A>{}.f();}
