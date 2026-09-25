// rule 7: expressions naming `super` are dependent: checked only when A:X is formed / the member is used
#include "common.h"
struct Only {int f() const {return 1;}};
struct Uses {
  int f() const {return 1+super::f();}
  int g() const {return super::missing();}   // no base has missing(): fine until g() is used
};
using U = Uses:Only;
int main() { U u; CHECK(u.f()==2); DONE("dependent"); }
