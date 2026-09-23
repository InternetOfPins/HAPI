// EXPECT-ERROR: member collision
#include "fx.h"
constexpr bool x = NoCollision<HapiMember_init,Chain<WithInitVoid,TerminalInitInt>>;
