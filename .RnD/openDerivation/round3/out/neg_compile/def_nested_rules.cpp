// expect: no D after C
// the Def nested in an outer rule walk: spliced like its APIOf, so C's rule sees the D that follows it
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
static_assert(hapi::BuildRules<hapi::Chain<>,hapi::Chain<Z,D>>::rules(), "outer walk");
int main() {}
