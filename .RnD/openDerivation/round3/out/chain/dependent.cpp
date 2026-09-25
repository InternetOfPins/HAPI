// rule 7: expressions naming `super` are dependent: checked only when A:X is formed / the member is used
#include "common.h"
struct Only {int f() const {return 1;}};
struct Uses {template<typename O> struct Part:O {
  using Base=O; using Base::Base;
  int f() const {return 1+Base::f();}
  int g() const {return Base::missing();}   // no base has missing(): fine until g() is used
};};
struct U : hapi::Chain<Uses>::Part<Only> {using Base=hapi::Chain<Uses>::Part<Only>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Uses,Only>>, "duplicate layer in U");};
int main() { U u; CHECK(u.f()==2); DONE("dependent"); }
