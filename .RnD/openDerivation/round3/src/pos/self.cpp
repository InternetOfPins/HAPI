// rule 1: inside the body, the class's own name means the family member (the Part), not bare A. In the struct-only form the
// family member is the layer under the named struct, so it is a base of X, not X itself (both lowering modes)
#include "common.h"
struct Term {int v=0; Term()=default; Term(int x):v(x) {}};
struct Self {
  using Me = Self;
  Self(int a,int b):super(a+b) {}
  Self& self() {return *this;}
  static int tag() {return 42;}
  int get() const {return super::v + Self::tag();}
};
struct X : Self:Term {};
struct Y : Self:Self:Term {};    // the same open class twice in one chain: two distinct layers
int main() {
  X x(1,2);                        CHECK(x.get()==45 && &x.self()==&x);
  static_assert(!std::is_same<X::Me,X>::value && std::is_base_of<X::Me,X>::value,
                "the injected name is the Part layer under the named struct (a base of X, not X)");
  IF_NESTED(static_assert(std::is_same<X::Me,X::Base>::value, "nested: it is exactly X's base");)
  Y y(1,2);                        CHECK(y.get()==45);          // inner Self: Term(3) via the inherited ctor; outer: v+tag()
  DONE("self");
}
