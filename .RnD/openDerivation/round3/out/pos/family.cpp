// rule 2: A:B is a family member of A, not a subtype; the derivation edge goes to B; every member has its own statics
#include "common.h"
struct B {int b=0; void has_B_method() {b=1;}};
struct C {};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; static inline int n=0; void touch() {++n; Base::has_B_method();}};};
struct AB : hapi::APIOf<B,A> {using Base=hapi::APIOf<B,A>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,B>>, "duplicate layer in AB");};
struct AC : hapi::APIOf<C,A> {using Base=hapi::APIOf<C,A>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,C>>, "duplicate layer in AC");};
int main() {
  AB ab; ab.has_B_method(); ab.touch();
  CHECK(ab.b==1 && AB::n==1 && AC::n==0);
  CHECK(&AB::n != &AC::n);
  B& r = ab; CHECK(&r.b==&ab.b);
  static_assert(std::is_base_of<B,AB>::value, "A:B derives from B");
  static_assert(!std::is_base_of<A,AB>::value && !std::is_convertible<AB*,A*>::value, "A:B is not an A");
  static_assert(!std::is_same<AB,AC>::value, "distinct family members");
  DONE("family");
}
