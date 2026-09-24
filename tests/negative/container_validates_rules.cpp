// EXPECT-ERROR: HAPI: validation failed
// A container whose Expand entry has `validates` on is spliced into the rule walk, so a failing rules() inside it
// now fires. This is the mechanism a per-container opt-in (plan D2) would use; the same component inside a
// container WITHOUT `validates` never runs (tests/expand_walks.cpp), which is today's behaviour for OneMenu's Menu.
#include "fx.h"
template<typename... II> struct V {template<typename O> struct Part : O {using O::O;};};
namespace hapi {
  template<typename... II> struct Expand<V<II...>> : Expansion<Chain<II...>,false,false,true,false> {};
}
APIOf<API,V<BadRule>> x;
