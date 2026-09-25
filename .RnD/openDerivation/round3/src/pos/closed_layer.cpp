// every operand without `final` is a layer, whatever its body says: a closed class (no `super`) keeps its definition, so bare
// use still works, and gets a Part carrying the same body. Data-only mixins need no `using super::super;`.
#include "common.h"
#include <hapi/hapi.h>
#include "layer_other.h"
struct T {int f() const {return 0;}};
struct A {int f() const {return 1+super::f();}};
struct Tag {int id=7;};                                   // data-only mixin
struct Counter {static inline int n=0; void inc() {++n;} Counter& self() {return *this;}};
struct Named {int v; Named(int x):v(x) {} Named():v(0) {}};
struct Ruled {                                            // rules() stays on the class, where HAPI's rule walk asks for it
  template<typename Before,typename After> static constexpr bool rules() {return true;}
  int r() const {return 3;}
};
struct K {int k() const {return 4;}};
struct Z  : Tag:A:final T {};
struct Z2 : Counter:A:final T {};
struct Z3 : Counter:Tag:final T {};                       // another family member: its own statics
struct Z4 : Named:A:final T {};
struct W  : A:Tag {};                                     // a component ending on a closed class
struct Z5 : W:final T {};
struct Z6 : Ruled:A:final T {};
struct Z7 : Mix:K:final T {};                             // closed classes from here and from another header
int main() {
  Z z; Z2 a; Z3 c; Z4 d(5); Z5 e; Z6 g; Z7 h; Tag bare; Counter bc;
  a.inc(); a.inc(); c.inc(); bc.inc();
  CHECK((z.id==7 && z.f()==1 && e.id==7 && e.f()==1 && d.v==5 && g.r()==3 && h.m==11 && h.k()==4));
  CHECK((Z2::n==2 && Z3::n==1 && Counter::n==1));         // rule 2: statics per family member, and bare Counter's own
  CHECK((bare.id==7 && Mix{}.m==11));                     // bare use still works
  static_assert(!std::is_base_of<Tag,Z>::value && !std::is_same<decltype(a.self()),Counter&>::value,
                "a family member, not a subtype; its own name rebinds to the layer");
  static_assert(hapi::HasRules<Ruled>::value, "rules() stays on the class");
  DONE("closed_layer");
}
