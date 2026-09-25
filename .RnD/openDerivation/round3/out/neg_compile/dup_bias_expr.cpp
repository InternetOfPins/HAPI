// expect: duplicate layer in Z
// Distinct: one type spelled two ways (Bias<1> and Bias<0+1>)
#include <hapi/hapi.h>
struct T {int f() const {return 0;}};
template<int k> struct Bias {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return k+Base::f();}};};
struct Z : hapi::APIOf<T,Bias<1>,Bias<0+1>> {using Base=hapi::APIOf<T,Bias<1>,Bias<0+1>>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Bias<1>,Bias<0+1>,T>>, "duplicate layer in Z");};
int main() {return Z{}.f();}
