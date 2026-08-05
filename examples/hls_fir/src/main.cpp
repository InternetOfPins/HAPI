/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief HLS DSP smoke test: an N-tap FIR filter where each tap is one
 *        Chain<> layer -- composition depth literally IS filter order,
 *        the same relationship hls_smoke shows between its 5 WrapWith<>
 *        layers and the folded adder. Unlike hls_smoke's stateless
 *        layers, each Tap<> here owns real state (a one-sample delay
 *        register), so this also exercises Chain<> composing stateful
 *        sections, not just pure functions.
 * @date 2026-08-05
 *
 */
#include <hapi/hapi.h>
#include <cstdint>
#include <type_traits>
using namespace hapi;

/// @brief innermost layer: no more taps, just closes the accumulation.
struct Item {
  static int32_t mac(int16_t /*delayed*/, int32_t acc) { return acc; }
};

/// @brief one FIR tap: one coefficient, one single-sample delay register.
/// On each call, contributes Coeff*z to the running sum (z holding the
/// sample this tap saw one call ago), latches the incoming sample into z,
/// and passes its own *old* z further down the chain -- so tap k of N,
/// composed in order, sees the input delayed by k calls. That is a real
/// shift register built entirely out of composition, not a shared array.
template<int16_t Coeff>
struct Tap {
  template<typename I>
  struct Part : I {
    using Base = I;
    using Base::Base;
    int16_t z{0};

    int32_t mac(int16_t x, int32_t acc) {
      int32_t sum  = acc + static_cast<int32_t>(Coeff) * static_cast<int32_t>(z);
      int16_t prev = z;
      z = x;
      return I::mac(prev, sum);
    }
  };
};

// 4-tap binomial smoothing kernel (Pascal's triangle row 3): 1 3 3 1
using Fir4 = Chain<Tap<1>, Tap<3>, Tap<3>, Tap<1>>;
using Top4 = APIOf<Item, Fir4>;

// 8-tap binomial smoothing kernel (Pascal's triangle row 7): 1 7 21 35 35 21 7 1
using Fir8 = Chain<Tap<1>, Tap<7>, Tap<21>, Tap<35>, Tap<35>, Tap<21>, Tap<7>, Tap<1>>;
using Top8 = APIOf<Item, Fir8>;

/// @brief same 4-tap kernel, but coefficients come from a runtime table
/// instead of a template NTTP -- models a runtime-configurable/adaptive
/// filter (coefficients genuinely unknown at synthesis time), see
/// hls/fir4_rtcoeff_top.cpp for why this variant exists.
volatile int16_t coeffTable[4] = {1, 3, 3, 1};

template<int Idx>
struct RTap {
  template<typename I>
  struct Part : I {
    using Base = I;
    using Base::Base;
    int16_t z{0};

    int32_t mac(int16_t x, int32_t acc) {
      int16_t coeff = coeffTable[Idx];
      int32_t sum   = acc + static_cast<int32_t>(coeff) * static_cast<int32_t>(z);
      int16_t prev  = z;
      z = x;
      return I::mac(prev, sum);
    }
  };
};

using Fir4Rt = Chain<RTap<0>, RTap<1>, RTap<2>, RTap<3>>;
using Top4Rt = APIOf<Item, Fir4Rt>;

/// @brief compile-time regression guards -- same spirit as hls_smoke's,
/// adjusted for the fact these layers are genuinely stateful.
static_assert(!std::is_polymorphic_v<Top4> && !std::is_polymorphic_v<Top8>,
  "HAPI regression: Chain/APIOf collapse must stay non-polymorphic (no vtable)");
static_assert(std::is_trivially_destructible_v<Top4> && std::is_trivially_destructible_v<Top8>,
  "HAPI regression: collapsed chain should stay trivially destructible");
/// Unlike hls_smoke's stateless WrapWith<>, every Tap<> layer adds exactly
/// one int16_t delay register -- so sizeof scales with tap count instead
/// of staying flat. Pinning the exact size catches any future layer
/// silently gaining extra padding/state.
static_assert(sizeof(Top4) == 4 * sizeof(int16_t),
  "HAPI regression: 4-tap chain should cost exactly 4 delay registers, no padding");
static_assert(sizeof(Top8) == 8 * sizeof(int16_t),
  "HAPI regression: 8-tap chain should cost exactly 8 delay registers, no padding");

Top4 fir4;
Top8 fir8;
Top4Rt fir4rt;

/// @brief the actual synthesis targets -- point an HLS backend's
/// top-function option here (e.g. bambu's --top-fname=firTop).
/// `x` is a genuine hardware input port: since it is not a compile-time
/// constant, none of the composed taps can be constant-folded away before
/// scheduling, so the generated RTL reflects the real Chain<> collapse,
/// called once per sample the way any real pipelined FIR would be.
int32_t fir4Top(int16_t x) { return fir4.mac(x, 0); }
int32_t fir8Top(int16_t x) { return fir8.mac(x, 0); }
int32_t fir4RtTop(int16_t x) { return fir4rt.mac(x, 0); }

#include <cstdio>
// host-side sanity check only -- not part of the synthesized design, same
// as hls_smoke's main(). Feeds an impulse (1 followed by zeros) into each
// filter: since every Tap<>'s z starts at 0, the impulse response of an
// N-tap FIR is just its own coefficients read back out, one per sample,
// delayed by one call (the pipeline's own one-sample latency through the
// first tap). Hand-checkable the same way hls_smoke's "888" is: the
// printed sequence must equal the coefficient list, shifted by one.
int main() {
  std::printf("fir4 impulse response (expect 0 1 3 3 1 0 0):\n");
  for (int i = 0; i < 7; ++i) {
    int16_t x = (i == 0) ? 1 : 0;
    std::printf("%d ", fir4Top(x));
  }
  std::printf("\n");

  std::printf("fir8 impulse response (expect 0 1 7 21 35 35 21 7 1 0 0):\n");
  for (int i = 0; i < 11; ++i) {
    int16_t x = (i == 0) ? 1 : 0;
    std::printf("%d ", fir8Top(x));
  }
  std::printf("\n");

  std::printf("fir4rt impulse response (expect 0 1 3 3 1 0 0):\n");
  for (int i = 0; i < 7; ++i) {
    int16_t x = (i == 0) ? 1 : 0;
    std::printf("%d ", fir4RtTop(x));
  }
  std::printf("\n");
  return 0;
}
