// round 1: one open class (names `super`), one closed terminal, one named composition (struct-only form)
#include <hapi/hapi.h>
#include <cstdio>

struct Id {                                   // closed: the last operand
  static int f(int x) {return x;}
  int n=7;
};

struct Twice {template<typename O> struct Part:O {                                // open: only usable as Twice:X
  using Base=O; using Base::Base;
  static int f(int x) {return 2*Base::f(x);}
  int twice_n() const {return 2*Base::n;}
};};

struct My : hapi::APIOf<Id,Twice> {using Base=hapi::APIOf<Id,Twice>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Twice,Id>>, "duplicate layer in My");}; using My_APIOf=hapi::APIOf<Id,Twice>;  namespace hapi { template<> struct Expand<My> : Expand<My_APIOf> {}; template<> struct HasOwnRules<My> : HasOwnRules<My_APIOf> {}; }

int main() {
  My m;
  std::printf("f=%d n=%d\n", My::f(21), m.twice_n());
  return My::f(21)==42 && m.twice_n()==14 ? 0 : 1;
}
