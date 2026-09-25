// amendment: no explicit termination. The rightmost operand may be open (names `super`): the chain then ends in the
// implicit empty terminal od::Nil. B's `super` call is fine as long as nothing uses it (rule 7: dependent).
#include "common.h"
struct B {int h() const {return 5;} int g() const {return super::missing();}};
struct A {int f() const {return 1+super::h();}};
struct Z : A:B {};
template<typename... PP> struct ZF : (PP : ... : B) {};
int main() {
  Z z; ZF<A> zf; ZF<> ze;
  CHECK(z.f()==6 && zf.f()==6 && ze.h()==5);
  static_assert(std::is_base_of<od::Nil,Z>::value && std::is_base_of<od::Nil,ZF<>>::value, "the chain ends in od::Nil");
  DONE("open_terminal");
}
