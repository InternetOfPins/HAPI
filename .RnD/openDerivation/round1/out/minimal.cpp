// round 1: one open class (names `super`), one closed terminal, one alias
#include <hapi/chain.h>
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

using My = hapi::Chain<Twice>::Part<Id>;

int main() {
  My m;
  std::printf("f=%d n=%d\n", My::f(21), m.twice_n());
  return My::f(21)==42 && m.twice_n()==14 ? 0 : 1;
}
