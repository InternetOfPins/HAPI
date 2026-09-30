// hapi/slots.h on AVR: typed slots (default) against the same state as a hand-indexed byte array (-DFLAT), one harness.
// tests/slots/run.sh builds both and requires the same program.
#include <hapi/hapi.h>
#include <stdint.h>
struct P { static constexpr int name() { return 1; } };
struct Q { static constexpr int name() { return 2; } };
struct SlotP { int16_t x; template<class Self, class V> static constexpr void each(Self& s, V& v) { v(s.x); } };
struct SlotQ { int16_t x; uint8_t y; template<class Self, class V> static constexpr void each(Self& s, V& v) { v(s.x); v(s.y); } };
using State = hapi::APIOf<hapi::SlotApi, hapi::Slot<P,SlotP>, hapi::Slot<Q,SlotQ>>::Res;      // Q is listed last: x at 0, y at 2; P's x at 3
volatile int16_t in_a; volatile uint8_t in_b; volatile int16_t out_a, out_c; volatile uint8_t out_b;
#ifdef FLAT
struct Flat { uint8_t v[5]; };
typedef int16_t __attribute__((may_alias)) i16a;
static inline int16_t rd(const uint8_t* p) { return *reinterpret_cast<const i16a*>(p); }
static inline void wr(uint8_t* p, int16_t x) { *reinterpret_cast<i16a*>(p) = x; }
extern "C" __attribute__((noinline)) void run(Flat* s) {
  wr(s->v + 0, int16_t(rd(s->v + 0) + rd(s->v + 3))); s->v[2] = uint8_t(s->v[2] ^ uint8_t(rd(s->v + 0))); wr(s->v + 3, int16_t(rd(s->v + 3) + 1)); }
int main() { Flat s; wr(s.v + 3, in_a); wr(s.v + 0, in_a); s.v[2] = in_b;
  for (;;) { run(&s); out_a = rd(s.v + 3); out_c = rd(s.v + 0); out_b = s.v[2]; } }
#else
extern "C" __attribute__((noinline)) void run(State* s) {
  hapi::slot<Q>(*s).x = int16_t(hapi::slot<Q>(*s).x + hapi::slot<P>(*s).x);
  hapi::slot<Q>(*s).y = uint8_t(hapi::slot<Q>(*s).y ^ uint8_t(hapi::slot<Q>(*s).x));
  hapi::slot<P>(*s).x = int16_t(hapi::slot<P>(*s).x + 1); }
int main() { State s; hapi::slot<P>(s).x = in_a; hapi::slot<Q>(s).x = in_a; hapi::slot<Q>(s).y = in_b;
  for (;;) { run(&s); out_a = hapi::slot<P>(s).x; out_c = hapi::slot<Q>(s).x; out_b = hapi::slot<Q>(s).y; } }
#endif
