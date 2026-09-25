// A closed struct is an XXXDef (derived from APIOf, as OneMenu's ItemDef<OO...>). HAPI keys Expand/HasOwnRules on the exact
// type, so the translator emits the documented forwarding entries: nested in another rule walk, the Def is spliced as its
// APIOf would be, and its components' rules see the enclosing context.
#include "common.h"
#include <hapi/hapi.h>
struct T {int f() const {return 0;}};
struct D {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return Base::f();}};};
struct C {template<typename O> struct Part:O {                                        // a rule about what may follow C in the enclosing walk
  using Base=O; using Base::Base;
  
  int f() const {return 1+Base::f();}
}; template<typename Before,typename After> static constexpr bool rules() {
    static_assert(!hapi::query<hapi::SameAs<D>,After>,"no D after C");
    return true;
  }};
struct Z : hapi::APIOf<T,C> {using Base=hapi::APIOf<T,C>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<C,T>>, "duplicate layer in Z");}; using Z_APIOf=hapi::APIOf<T,C>;  namespace hapi { template<> struct Expand<Z> : Expand<Z_APIOf> {}; template<> struct HasOwnRules<Z> : HasOwnRules<Z_APIOf> {}; }                          // an XXXDef: derived from APIOf<T,C>, with its hapi::Expand entry
struct Bare : hapi::APIOf<T,C> {};                // the same derivation written by hand, without the entry: a leaf
static_assert(hapi::Validates<Z>::value && std::is_same<hapi::Expand<Z>::Children,hapi::Chain<T,C>>::value,
              "Z expands as APIOf<T,C> does (validates, children T,C)");
static_assert(!hapi::HasOwnRules<Z>::value, "and is never probed for rules of its own, like APIOf");
static_assert(!hapi::Validates<Bare>::value && hapi::BuildRules<hapi::Chain<>,hapi::Chain<Bare,D>>::rules(),
              "without the entry, a derived struct is a leaf: C's rule never sees the D after it");
static_assert(hapi::BuildRules<hapi::Chain<>,hapi::Chain<Z,T>>::rules(), "nested, with nothing forbidden after it");
int main() { CHECK(Z{}.f()==1); DONE("def_expand"); }
