// EXPECT-ERROR: member collision
// the nested Chain is NOT in head position, so the head-only splice does not apply; the collision is still found
#include "fx.h"
constexpr bool x = NoCollision<HapiMember_init,Chain<TerminalInitInt,Chain<WithInitVoid>>>;
