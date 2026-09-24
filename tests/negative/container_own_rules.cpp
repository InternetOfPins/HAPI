// EXPECT-ERROR: HAPI: validation failed
// A container with `validates` on is spliced into the rule walk by its children, but its OWN rules() must still run,
// exactly as if it were placed directly. Here VO's rule (no A after it) fails on the sibling A. If the splice dropped
// the container's own rule this would compile silently (that is what happened to OneMenu's ItemPrinter before the fix).
#include "fx.h"
template<typename... II> struct VO {
  template<typename Bf,typename Af> static constexpr bool rules() {return !Requires<SameAs<A>,Af>;}
  template<typename O> struct Part : O {using O::O;};
};
namespace hapi {
  template<typename... II> struct Expand<VO<II...>> : Expansion<Chain<II...>,false,false,true,false> {};
}
APIOf<API,VO<>,A> x;
