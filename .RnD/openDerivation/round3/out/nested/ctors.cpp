// rule 6: every layer inherits its base's constructors, transitively along a chain, and so does the named struct (the
// translator injects `using Base::Base;`: no user `using` below); A's same-signature constructor hides the inherited one
#include "common.h"
struct Term   {int v; Term(int x):v(x) {} Term(int x,int y):v(x*y) {}};
struct Pass   {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int get() const {return Base::v;}};};
struct Pass2  {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int twice() const {return 2*Base::v;}};};
struct Offset {template<typename O> struct Part:O {
 using Base=O; using Base::Base; Part(int x):Base(x+1000) {} int get() const {return Base::v;}};};
struct Explicit {template<typename O> struct Part:O {
 using Base=O; using Base::Base;   int get() const {return Base::v;}};};   // writing rule 6 out is allowed, and redundant

struct X : Pass::Part<Pass2::Part<Term>> {using Base=Pass::Part<Pass2::Part<Term>>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Pass,Pass2,Term>>, "duplicate layer in X");};
struct Y : Offset::Part<Pass2::Part<Term>> {using Base=Offset::Part<Pass2::Part<Term>>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Offset,Pass2,Term>>, "duplicate layer in Y");};
struct E : Explicit::Part<Term> {using Base=Explicit::Part<Term>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Explicit,Term>>, "duplicate layer in E");};

int main() {
  X a(5), b(3,4);     CHECK(a.get()==5 && a.twice()==10 && b.get()==12);
  Y c(5), d(3,4);     CHECK(c.get()==1005 && d.get()==12);    // Offset(int) hides Term(int); Term(int,int) still inherited
  E e(7);             CHECK(e.get()==7);
  static_assert(!std::is_default_constructible<X>::value, "Term has no default constructor, neither does X");
  DONE("ctors");
}
