// rule 7: expressions naming `super` are dependent: checked only when A:X is formed / the member is used
#include "common.h"
struct Only {int f() const {return 1;}};
struct Uses {template<typename O> struct Part:O {
  using Base=O; using Base::Base;
  int f() const {return 1+Base::f();}
  int g() const {return Base::missing();}   // no base has missing(): fine until g() is used
};};
struct U : hapi::APIOf<Only,Uses> {using Base=hapi::APIOf<Only,Uses>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Uses,Only>>, "duplicate layer in U");}; using U_APIOf=hapi::APIOf<Only,Uses>;  namespace hapi { template<> struct Expand< ::U> : Expand< ::U_APIOf> {}; template<> struct HasOwnRules< ::U> : HasOwnRules< ::U_APIOf> {}; }
int main() { U u; CHECK(u.f()==2); DONE("dependent"); }
