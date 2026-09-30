// EXPECT-ERROR: hapi::Slot: two Parts claim the same tag
// the tag is repeated across a nested APIOf
#include "../../include/hapi/hapi.h"
using namespace hapi;
struct A { static constexpr int name() { return 1; } };
struct D { static constexpr int name() { return 4; } };
struct S1 { int x; template<class Self, class V> static constexpr void each(Self& s, V& v) { v(s.x); } };
using Bad = APIOf<SlotApi, Slot<A,S1>, APIOf<SlotApi, Slot<A,S1>>, Slot<D,S1>>::Res;
Bad b;
