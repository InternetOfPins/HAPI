// struct A : B:C {...}  <=>  struct C {..}; struct B : C {..}; struct A : B {..};
// inside the named struct `super` is its base (B:C), and the base's constructors are inherited
#include "common.h"
struct C  {int v=1; int get() const {return v;}};
struct B  {int get() const {return 2*super::get();}};
struct A  : B:C {int get() const {return 1+super::get();}};

struct C2 {int v=1; int get() const {return v;}};         // the same, in plain C++
struct B2 : C2 {int get() const {return 2*C2::get();}};
struct A2 : B2 {int get() const {return 1+B2::get();}};

template<typename T> struct W : B:T {int get() const {return 100+super::get();}};   // dependent base
struct K  {int v; K(int x):v(x) {} int get() const {return v;}};
struct AK : B:K {int get() const {return 1+super::get();}};                          // K(int) reaches AK

int main() {
  A a; A2 a2;
  CHECK(a.get()==3 && a2.get()==3);
  CHECK(W<C>{}.get()==102);
  AK ak(5); CHECK(ak.get()==11);
  static_assert(std::is_base_of<C,A>::value && std::is_base_of<C2,A2>::value, "C is a base of A, as in plain C++");
  static_assert(!std::is_same<A,A2>::value, "and A is its own type");
  DONE("named_chain");
}
