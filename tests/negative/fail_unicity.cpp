// EXPECT-ERROR: do not repeat B!
#include "fx.h"
constexpr ItemDef<A,B,B> x{};
