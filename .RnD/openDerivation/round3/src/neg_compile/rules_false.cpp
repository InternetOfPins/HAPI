// expect: HAPI: validation failed in Z
// a component whose rules() just answers false: the composed struct's own static_assert names it
#include <hapi/rules.h>
struct T {int f() const {return 0;}};
struct Never {
  template<typename Before,typename After> static constexpr bool rules() {return false;}
  int f() const {return super::f();}
};
struct Z : Never:T {};
