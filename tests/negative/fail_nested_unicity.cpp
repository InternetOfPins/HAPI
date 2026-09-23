// EXPECT-ERROR: do not repeat B!
#include "fx.h"
using ClosedAB = APIOf<A,B>; constexpr APIOf<ClosedAB,B> x{};
