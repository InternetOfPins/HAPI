// expect: duplicate layer in Z
// Distinct: one type spelled two ways (Bias<1> and Bias<0+1>)
#include <hapi/hapi.h>
struct T {int f() const {return 0;}};
template<int k> struct Bias {int f() const {return k+super::f();}};
struct Z : Bias<1>:Bias<0+1>:final T {};
int main() {return Z{}.f();}
