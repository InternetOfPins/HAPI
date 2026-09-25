// expect: duplicate layer in Z
// Distinct (instantiation time): a duplicate that only exists once the pack is known; the text check cannot see OO
#include <hapi/hapi.h>
struct T {int f() const {return 0;}};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 1+Base::f();}};};
template<typename... OO> struct Z : hapi::APIOf<T,OO...> {using Base=hapi::APIOf<T,OO...>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<OO...,T>>, "duplicate layer in Z");};
int main() {return Z<A,A>{}.f();}
