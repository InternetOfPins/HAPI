// rule 2: A:B is a family member of A, not a subtype; the derivation edge goes to B; every member has its own statics
#include "common.h"
struct B {int b=0; void has_B_method() {b=1;}};
struct C {};
struct A {static inline int n=0; void touch() {++n; super::has_B_method();}};
struct AB : A:final B {};
struct AC : A:final C {};
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
