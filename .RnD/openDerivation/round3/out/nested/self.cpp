// rule 1: inside the body, the class's own name means the family member (the Part), not bare A. In the struct-only form the
// family member is the layer under the named struct, so it is a base of X, not X itself (both lowering modes)
#include "common.h"
struct Term {int v=0; Term()=default; Term(int x):v(x) {}};
struct Self {template<typename O> struct Part:O {
  using Base=O; using Base::Base;
  using Me = Part;
  Part(int a,int b):Base(a+b) {}
  Part& self() {return *this;}
  static int tag() {return 42;}
  int get() const {return Base::v + Part::tag();}
};};
struct X : Self::Part<Term> {using Base=Self::Part<Term>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Self,Term>>, "duplicate layer in X"); static_assert(hapi::BuildRules<hapi::Chain<>,hapi::Chain<Term,Self>>::rules(), "HAPI: validation failed in X");};
int main() {
  X x(1,2);                        CHECK(x.get()==45 && &x.self()==&x);
  static_assert(!std::is_same<X::Me,X>::value && std::is_base_of<X::Me,X>::value,
                "the injected name is the Part layer under the named struct (a base of X, not X)");
  IF_NESTED(static_assert(std::is_same<X::Me,X::Base>::value, "nested: it is exactly X's base");)
  DONE("self");
}
