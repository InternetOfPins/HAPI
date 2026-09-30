// EXPECT-ERROR: hapi::slot<Tag>: no slot with that tag
#include "../../include/hapi/hapi.h"
using namespace hapi;
struct A { static constexpr int name() { return 1; } };
struct Z { static constexpr int name() { return 26; } };
struct S1 { int x; template<class Self, class V> static constexpr void each(Self& s, V& v) { v(s.x); } };
using State = APIOf<SlotApi, Slot<A,S1>>::Res;
int f(State& s) { return slot<Z>(s).x; }
