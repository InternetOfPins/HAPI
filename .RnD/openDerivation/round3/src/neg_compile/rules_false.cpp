// expect: HAPI: validation failed
// a component whose rules() just answers false: APIOf's own rule walk rejects it
#include <hapi/hapi.h>
struct T {int f() const {return 0;}};
struct Never {
  template<typename Before,typename After> static constexpr bool rules() {return false;}
  int f() const {return super::f();}
};
struct Z : Never:final T {};
