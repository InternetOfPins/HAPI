/**
 * @file slots_tests.cpp
 * @brief hapi/slots.h: tag-addressed slots along a chain, nested compositions, the Contract parameter.
 *
 * Compiled and run; a failing check makes the exit status non-zero.
 */
#include <hapi/hapi.h>
#include <stdint.h>
#include <stdio.h>
using namespace hapi;

namespace slots_test {
  // tags: the core does not know what a name is, here an int
  struct A { static constexpr int name() { return 11; } };
  struct B { static constexpr int name() { return 22; } };
  struct C { static constexpr int name() { return 33; } };
  struct D { static constexpr int name() { return 44; } };
  struct Empty { static constexpr int name() { return 55; } };

  struct SlotA { int16_t x; uint8_t y; template<class Self, class V> static constexpr void each(Self& s, V& v) { v(s.x); v(s.y); } };
  struct SlotB { int16_t x; template<class Self, class V> static constexpr void each(Self& s, V& v) { v(s.x); } };   // a field named like SlotA's: no clash
  struct SlotC { int16_t x; template<class Self, class V> static constexpr void each(Self& s, V& v) { v(s.x); } };
  struct SlotD { int16_t x; template<class Self, class V> static constexpr void each(Self& s, V& v) { v(s.x); } };
  struct SlotNone { template<class Self, class V> static constexpr void each(Self&, V&) {} };

  using State = APIOf<SlotApi, Slot<A,SlotA>, Slot<B,SlotB>, Slot<Empty,SlotNone>>::Res;
  static_assert( HasSlot<A,State>::value && HasSlot<B,State>::value && HasSlot<Empty,State>::value, "a slot is found by its tag");
  static_assert(!HasSlot<C,State>::value, "a tag that is not in the state is not found");

  // a Contract adds members to the state's type (this is the example of hapi/slots.h)
  template<class Below, class R> struct Counted : Below { unsigned steps = 0; void step() { ++steps; } };
  using Counting = APIOf<SlotApi, Slot<A,SlotA,Counted>>::Res;

  // nested compositions: a nested APIOf (once and twice) and a nested Chain, as one component of an outer chain
  using Inner = APIOf<SlotApi, Slot<B,SlotB>, Slot<C,SlotC>>;
  using Pair  = Chain<Slot<B,SlotB>, Slot<C,SlotC>>;
  using Flat      = APIOf<SlotApi, Slot<A,SlotA>, Slot<B,SlotB>, Slot<C,SlotC>, Slot<D,SlotD>>::Res;
  using WithApiOf = APIOf<SlotApi, Slot<A,SlotA>, Inner, Slot<D,SlotD>>::Res;
  using WithChain = APIOf<SlotApi, Slot<A,SlotA>, Pair, Slot<D,SlotD>>::Res;
  using Twice     = APIOf<SlotApi, Slot<A,SlotA>, APIOf<SlotApi, APIOf<SlotApi, Slot<B,SlotB>>, Slot<C,SlotC>>, Slot<D,SlotD>>::Res;

  struct Collect {
    int layers[8]; int nl = 0; int vals[16]; int nv = 0;
    void layer(int n) { layers[nl++] = n; }
    template<class T> void operator()(const T& x) { vals[nv++] = int(x); }
  };
  int failures = 0;
  void check(const char* name, bool ok) { printf("CHECK %s: %s\n", name, ok ? "ok" : "FAIL"); if (!ok) failures++; }

  template<class R> bool reachable(R& r) {
    slot<A>(r).x = 1; slot<B>(r).x = 2; slot<C>(r).x = 3; slot<D>(r).x = 4;
    return slot<A>(r).x == 1 && slot<B>(r).x == 2 && slot<C>(r).x == 3 && slot<D>(r).x == 4;
  }
  template<class R> bool same_walk(R& r) {
    Flat f{}; reachable(r); reachable(f);
    Collect a, b; r.each(a); f.each(b);
    bool same = a.nl == b.nl && a.nv == b.nv;
    for (int i = 0; same && i < a.nl; i++) same = a.layers[i] == b.layers[i];
    for (int i = 0; same && i < a.nv; i++) same = a.vals[i] == b.vals[i];
    return same;
  }
}

int main() {
  using namespace slots_test;
  State s{}; slot<A>(s).x = 1; slot<B>(s).x = 2; slot<A>(s).y = 3;
  check("tags", slot<A>(s).x == 1 && slot<B>(s).x == 2 && slot<A>(s).y == 3);

  { using One = APIOf<SlotApi, Slot<A,SlotA>>::Res; const One c = One{{}, {SlotA{5, 6}}};
    check("brace-init", slot<A>(c).x == 5 && slot<A>(c).y == 6); }             // a state of one slot is { {}, {slot} }

  { Collect v; s.each(v);
    check("each-order", v.nl == 3 && v.layers[0] == 55 && v.layers[1] == 22 && v.layers[2] == 11 && v.nv == 3 && v.vals[0] == 2 && v.vals[1] == 1 && v.vals[2] == 3); }   // last-listed first
  { const State c = s; Collect v; c.each(v); check("each-const", v.nv == 3); }

  { Counting c{}; slot<A>(c).x = 21; c.step(); c.step();
    check("contract", c.steps == 2 && slot<A>(c).x == 21); }

  { WithApiOf a{}; WithChain b{}; Twice t{};
    check("nested-apiof", reachable(a));
    check("nested-chain", reachable(b));
    check("nested-twice", reachable(t));
    check("nested-walk-as-flat", same_walk(a) && same_walk(b) && same_walk(t)); }

  // sizes: an empty slot and nesting cost nothing on GCC and Clang; on MSVC nesting still costs nothing but an empty slot costs 2 bytes (measured in CI), so only the sizes are printed there
  {
    using Without = APIOf<SlotApi, Slot<A,SlotA>, Slot<B,SlotB>>::Res;
#ifdef _MSC_VER
    printf("note: MSVC sizeof: with an empty slot %zu, without %zu; nested %zu, flat %zu\n", sizeof(State), sizeof(Without), sizeof(WithApiOf), sizeof(Flat));
#else
    check("empty-costs-nothing", sizeof(State) == sizeof(Without));
    check("nesting-costs-nothing", sizeof(WithApiOf) == sizeof(Flat) && sizeof(WithChain) == sizeof(Flat) && sizeof(Twice) == sizeof(Flat));
#endif
  }
  printf("%d failed\n", failures);
  return failures;
}
