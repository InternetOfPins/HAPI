// rule 1: inside the body, the class's own name means A:B (the family member), not bare A
#include "common.h"
struct Term {int v=0; Term()=default; Term(int x):v(x) {}};
struct Self {
  using Me = Self;
  Self(int a,int b):super(a+b) {}
  Self& self() {return *this;}
  static int tag() {return 42;}
  int get() const {return super::v + Self::tag();}
};
using X = Self:Term;
using Y = Self:Self:Term;       // the same open class twice in one chain: two distinct layers
int main() {
  X x(1,2);                        CHECK(x.get()==45 && &x.self()==&x);
  IF_NESTED(static_assert(std::is_same<X::Me,X>::value, "nested: the injected name is exactly A:B");)
  IF_CHAIN(static_assert(!std::is_same<X::Me,X>::value && std::is_base_of<X::Me,X>::value,
                         "chain: the injected name is the Part under the Chain wrapper (a base of X, not X)");)
  Y y(1,2);                        CHECK(y.get()==45);          // inner Self: Term(3) via the inherited ctor; outer: v+tag()
  DONE("self");
}
