// rule 4: ':' is only derivation in an alias RHS and a base clause. Written as a declaration in a block, `A:B x;` is
// already valid C++ with another meaning (label A, then `B x;`), so the translator leaves it alone and it compiles.
#include "common.h"
struct B {int b=7;};
struct A {using super::super;};
int main() {
  A:B x;
  static_assert(std::is_same<decltype(x),B>::value, "`A:B x;` in a block is the label A: followed by `B x;`");
  CHECK(x.b==7);
  DONE("label_hazard");
}
