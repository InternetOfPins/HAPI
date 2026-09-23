// EXPECT-ERROR: A must be before B
#include "fx.h"
constexpr ItemDef<B,A> x{};
