// expect: HAPI: validation failed
// a component whose rules() just answers false: APIOf's own rule walk rejects it
#include <hapi/hapi.h>
struct T {int f() const {return 0;}};
struct Never {template<typename O> struct Part:O {
  using Base=O; using Base::Base;
  
  int f() const {return Base::f();}
}; template<typename Before,typename After> static constexpr bool rules() {return false;}};
struct Z : hapi::APIOf<T,Never> {using Base=hapi::APIOf<T,Never>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Never,T>>, "duplicate layer in Z");};
