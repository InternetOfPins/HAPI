// expect: duplicate layer in Z
// Distinct: one type spelled two ways (Bias<1> and Bias<0+1>)
#include <hapi/rules.h>
struct T {int f() const {return 0;}};
template<int k> struct Bias {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return k+Base::f();}};};
struct Z : hapi::Chain<Bias<1>,Bias<0+1>>::Part<T> {using Base=hapi::Chain<Bias<1>,Bias<0+1>>::Part<T>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Bias<1>,Bias<0+1>,T>>, "duplicate layer in Z"); static_assert(hapi::BuildRules<hapi::Chain<>,hapi::Chain<T,Bias<1>,Bias<0+1>>>::rules(), "HAPI: validation failed in Z");};
int main() {return Z{}.f();}
