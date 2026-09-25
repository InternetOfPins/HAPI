// expect: no D after C
// the Def nested in an outer rule walk: spliced like its APIOf, so C's rule sees the D that follows it
#include <hapi/hapi.h>
struct T {int f() const {return 0;}};
struct D {int f() const {return super::f();}};
struct C {                                        // a rule about what may follow C in the enclosing walk
  template<typename Before,typename After> static constexpr bool rules() {
    static_assert(!hapi::query<hapi::SameAs<D>,After>,"no D after C");
    return true;
  }
  int f() const {return 1+super::f();}
};
struct Z : C:final T {};                          // an XXXDef: derived from APIOf<T,C>, with its hapi::Expand entry
static_assert(hapi::BuildRules<hapi::Chain<>,hapi::Chain<Z,D>>::rules(), "outer walk");
int main() {}
