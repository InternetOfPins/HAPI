// EXPECT-ERROR: hapi::Slot: two Parts claim the same tag
#include "../../include/hapi/hapi.h"
using namespace hapi;
struct A { static constexpr int name() { return 1; } };
struct S1 { int x; template<class Self, class V> static constexpr void each(Self& s, V& v) { v(s.x); } };
struct S2 { int y; template<class Self, class V> static constexpr void each(Self& s, V& v) { v(s.y); } };
using Bad = APIOf<SlotApi, Slot<A,S1>, Slot<A,S2>>::Res;
Bad b;
