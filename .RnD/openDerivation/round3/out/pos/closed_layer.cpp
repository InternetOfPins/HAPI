// every operand without `final` is a layer, whatever its body says: a closed class (no `super`) keeps its definition, so bare
// use still works, and gets a Part carrying the same body. Data-only mixins need no `using super::super;`.
#include "common.h"
#include <hapi/hapi.h>
#include "layer_other.h"
struct T {int f() const {return 0;}};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 1+Base::f();}};};
struct Tag {int id=7; template<typename O> struct Part:O {using Base=O; using Base::Base; int id=7;}; };                                   // data-only mixin
struct Counter {static inline int n=0; void inc() {++n;} Counter& self() {return *this;} template<typename O> struct Part:O {using Base=O; using Base::Base; static inline int n=0; void inc() {++n;} Part& self() {return *this;}}; };
struct Named {int v; Named(int x):v(x) {} Named():v(0) {} template<typename O> struct Part:O {using Base=O; using Base::Base; int v; Part(int x):v(x) {} Part():v(0) {}}; };
struct Ruled {                                            // rules() stays on the class, where HAPI's rule walk asks for it
  template<typename Before,typename After> static constexpr bool rules() {return true;}
  int r() const {return 3;}
 template<typename O> struct Part:O {using Base=O; using Base::Base;
  int r() const {return 3;}
}; };
struct K {int k() const {return 4;} template<typename O> struct Part:O {using Base=O; using Base::Base; int k() const {return 4;}}; };
struct Z  : hapi::APIOf<T,Tag,A> {using Base=hapi::APIOf<T,Tag,A>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Tag,A,T>>, "duplicate layer in Z");}; using Z_APIOf=hapi::APIOf<T,Tag,A>;  namespace hapi { template<> struct Expand< ::Z> : Expand< ::Z_APIOf> {}; template<> struct HasOwnRules< ::Z> : HasOwnRules< ::Z_APIOf> {}; }
struct Z2 : hapi::APIOf<T,Counter,A> {using Base=hapi::APIOf<T,Counter,A>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Counter,A,T>>, "duplicate layer in Z2");}; using Z2_APIOf=hapi::APIOf<T,Counter,A>;  namespace hapi { template<> struct Expand< ::Z2> : Expand< ::Z2_APIOf> {}; template<> struct HasOwnRules< ::Z2> : HasOwnRules< ::Z2_APIOf> {}; }
struct Z3 : hapi::APIOf<T,Counter,Tag> {using Base=hapi::APIOf<T,Counter,Tag>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Counter,Tag,T>>, "duplicate layer in Z3");}; using Z3_APIOf=hapi::APIOf<T,Counter,Tag>;  namespace hapi { template<> struct Expand< ::Z3> : Expand< ::Z3_APIOf> {}; template<> struct HasOwnRules< ::Z3> : HasOwnRules< ::Z3_APIOf> {}; }                       // another family member: its own statics
struct Z4 : hapi::APIOf<T,Named,A> {using Base=hapi::APIOf<T,Named,A>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Named,A,T>>, "duplicate layer in Z4");}; using Z4_APIOf=hapi::APIOf<T,Named,A>;  namespace hapi { template<> struct Expand< ::Z4> : Expand< ::Z4_APIOf> {}; template<> struct HasOwnRules< ::Z4> : HasOwnRules< ::Z4_APIOf> {}; }
struct W  : hapi::Chain<A,Tag> {static_assert(hapi::Distinct<hapi::Chain<A,Tag>>, "duplicate layer in W");};                                     // a component ending on a closed class
struct Z5 : hapi::APIOf<T,W> {using Base=hapi::APIOf<T,W>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<W,T>>, "duplicate layer in Z5");}; using Z5_APIOf=hapi::APIOf<T,W>;  namespace hapi { template<> struct Expand< ::Z5> : Expand< ::Z5_APIOf> {}; template<> struct HasOwnRules< ::Z5> : HasOwnRules< ::Z5_APIOf> {}; }
struct Z6 : hapi::APIOf<T,Ruled,A> {using Base=hapi::APIOf<T,Ruled,A>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Ruled,A,T>>, "duplicate layer in Z6");}; using Z6_APIOf=hapi::APIOf<T,Ruled,A>;  namespace hapi { template<> struct Expand< ::Z6> : Expand< ::Z6_APIOf> {}; template<> struct HasOwnRules< ::Z6> : HasOwnRules< ::Z6_APIOf> {}; }
struct Z7 : hapi::APIOf<T,Mix,K> {using Base=hapi::APIOf<T,Mix,K>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Mix,K,T>>, "duplicate layer in Z7");}; using Z7_APIOf=hapi::APIOf<T,Mix,K>;  namespace hapi { template<> struct Expand< ::Z7> : Expand< ::Z7_APIOf> {}; template<> struct HasOwnRules< ::Z7> : HasOwnRules< ::Z7_APIOf> {}; }                             // closed classes from here and from another header
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
