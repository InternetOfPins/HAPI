// expect: duplicate layer in Y
// A over a component X that already holds A: rejected by the compiler (hapi::Distinct splices X's Types)
#include <hapi/hapi.h>
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 1+Base::f();}};};
struct B {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 2+Base::f();}};};
struct C {int f() const {return 0;}};
struct X : hapi::Chain<A,B> {static_assert(hapi::Distinct<hapi::Chain<A,B>>, "duplicate layer in X");};
struct Y : hapi::APIOf<C,A,X> {using Base=hapi::APIOf<C,A,X>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,X,C>>, "duplicate layer in Y");}; using Y_APIOf=hapi::APIOf<C,A,X>;  namespace hapi { template<> struct Expand< ::Y> : Expand< ::Y_APIOf> {}; template<> struct HasOwnRules< ::Y> : HasOwnRules< ::Y_APIOf> {}; }
