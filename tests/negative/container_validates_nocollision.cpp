// EXPECT-ERROR: member collision
// Same for NoCollision: a colliding member hidden inside a container with `validates` is found (head position;
// see the [gap] pin in tests/descent_characterization.cpp for why tail position is not).
#include "fx.h"
template<typename... II> struct V {template<typename O> struct Part : O {using O::O;};};
namespace hapi {
  template<typename... II> struct Expand<V<II...>> : Expansion<Chain<II...>,false,false,true,false> {};
}
constexpr bool x = NoCollision<HapiMember_init,Chain<V<TerminalInitInt>,WithInitVoid>>;
