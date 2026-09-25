// expect: error
// the component's B calls super::missing(); closed on the user's Nil, using it fails (Nil has no missing)
#include <hapi/hapi.h>
struct Nil {};
struct B {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int h() const {return 5;} int g() const {return Base::missing();}};};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 1+Base::h();}};};
struct W : hapi::Chain<A,B> {static_assert(hapi::Distinct<hapi::Chain<A,B>>, "duplicate layer in W");};
struct Z : hapi::APIOf<Nil,W> {using Base=hapi::APIOf<Nil,W>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<W,Nil>>, "duplicate layer in Z");}; using Z_APIOf=hapi::APIOf<Nil,W>;  namespace hapi { template<> struct Expand<Z> : Expand<Z_APIOf> {}; template<> struct HasOwnRules<Z> : HasOwnRules<Z_APIOf> {}; }
int main() {return Z{}.g();}
