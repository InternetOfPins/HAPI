// rule 6: A:B implicitly has `using B::B`, transitively along a chain; A's same-signature constructor hides the inherited one
#include "common.h"
struct Term   {int v; Term(int x):v(x) {} Term(int x,int y):v(x*y) {}};
struct Pass   {int get() const {return super::v;}};
struct Pass2  {int twice() const {return 2*super::v;}};
struct Offset {Offset(int x):super(x+1000) {} int get() const {return super::v;}};
struct Explicit {using super::super; int get() const {return super::v;}};   // writing rule 6 out is allowed, and redundant

using X = Pass:Pass2:Term;
using Y = Offset:Pass2:Term;
using E = Explicit:Term;

int main() {
  X a(5), b(3,4);     CHECK(a.get()==5 && a.twice()==10 && b.get()==12);
  Y c(5), d(3,4);     CHECK(c.get()==1005 && d.get()==12);    // Offset(int) hides Term(int); Term(int,int) still inherited
  E e(7);             CHECK(e.get()==7);
  static_assert(!std::is_default_constructible<X>::value, "Term has no default constructor, neither does X");
  DONE("ctors");
}
