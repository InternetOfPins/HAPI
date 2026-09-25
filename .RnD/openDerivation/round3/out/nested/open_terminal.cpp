// amendment: no explicit termination. The rightmost operand may be open (names `super`): the chain then ends in the
// implicit empty terminal od::Nil. B's `super` call is fine as long as nothing uses it (rule 7: dependent).
#include "common.h"
struct B {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int h() const {return 5;} int g() const {return Base::missing();}};};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 1+Base::h();}};};
struct Z : A::Part<B::Part<od::Nil>> {using Base=A::Part<B::Part<od::Nil>>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,B>>, "duplicate layer in Z");};
template<typename... PP> struct ZF : od::FoldT<od::Nil,PP...,B> {using Base=typename od::FoldT<od::Nil,PP...,B>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<PP...,B>>, "duplicate layer in ZF");};
int main() {
  Z z; ZF<A> zf; ZF<> ze;
  CHECK(z.f()==6 && zf.f()==6 && ze.h()==5);
  static_assert(std::is_base_of<od::Nil,Z>::value && std::is_base_of<od::Nil,ZF<>>::value, "the chain ends in od::Nil");
  DONE("open_terminal");
}
