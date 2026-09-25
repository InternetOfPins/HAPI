// round 1: one open class (names `super`), one closed terminal, one alias
#include <hapi/chain.h>
#include <cstdio>

struct Id {                                   // closed: the last operand
  static int f(int x) {return x;}
  int n=7;
};

struct Twice {                                // open: only usable as Twice:X
  static int f(int x) {return 2*super::f(x);}
  int twice_n() const {return 2*super::n;}
};

using My = Twice:Id;

int main() {
  My m;
  std::printf("f=%d n=%d\n", My::f(21), m.twice_n());
  return My::f(21)==42 && m.twice_n()==14 ? 0 : 1;
}
