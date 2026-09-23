// EXPECT-ERROR: B only makes sense after A
#include "fx.h"
constexpr ItemDef<B> x{};
