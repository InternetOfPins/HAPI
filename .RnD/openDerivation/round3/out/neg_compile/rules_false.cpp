// expect: HAPI: validation failed in Z
// a component whose rules() just answers false: the composed struct's own static_assert names it
#include <hapi/rules.h>
struct T {int f() const {return 0;}};
struct Never {template<typename O> struct Part:O {
  using Base=O; using Base::Base;
  
  int f() const {return Base::f();}
}; template<typename Before,typename After> static constexpr bool rules() {return false;}};
struct Z : hapi::Chain<Never>::Part<T> {using Base=hapi::Chain<Never>::Part<T>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Never,T>>, "duplicate layer in Z"); static_assert(hapi::BuildRules<hapi::Chain<>,hapi::Chain<T,Never>>::rules(), "HAPI: validation failed in Z");};
