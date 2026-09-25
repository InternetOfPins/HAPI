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

struct X : hapi::APIOf<Term,Pass,Pass2> {using Base=hapi::APIOf<Term,Pass,Pass2>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Pass,Pass2,Term>>, "duplicate layer in X");}; using X_APIOf=hapi::APIOf<Term,Pass,Pass2>;  namespace hapi { template<> struct Expand<X> : Expand<X_APIOf> {}; template<> struct HasOwnRules<X> : HasOwnRules<X_APIOf> {}; }
struct Y : hapi::APIOf<Term,Offset,Pass2> {using Base=hapi::APIOf<Term,Offset,Pass2>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Offset,Pass2,Term>>, "duplicate layer in Y");}; using Y_APIOf=hapi::APIOf<Term,Offset,Pass2>;  namespace hapi { template<> struct Expand<Y> : Expand<Y_APIOf> {}; template<> struct HasOwnRules<Y> : HasOwnRules<Y_APIOf> {}; }
struct E : hapi::APIOf<Term,Explicit> {using Base=hapi::APIOf<Term,Explicit>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Explicit,Term>>, "duplicate layer in E");}; using E_APIOf=hapi::APIOf<Term,Explicit>;  namespace hapi { template<> struct Expand<E> : Expand<E_APIOf> {}; template<> struct HasOwnRules<E> : HasOwnRules<E_APIOf> {}; }

int main() {
  X a(5), b(3,4);     CHECK(a.get()==5 && a.twice()==10 && b.get()==12);
  Y c(5), d(3,4);     CHECK(c.get()==1005 && d.get()==12);    // Offset(int) hides Term(int); Term(int,int) still inherited
  E e(7);             CHECK(e.get()==7);
  static_assert(!std::is_default_constructible<X>::value, "Term has no default constructor, neither does X");
  DONE("ctors");
}
