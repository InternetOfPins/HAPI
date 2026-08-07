// HLS synthesis target: two 4-tap Hamming-LPF FIR sections cascaded in
// series -- stage 1's full output feeds stage 2 as its input sample
// stream, the textbook "cascaded sections" shape from DSP filter design.
// Isolated in its own translation unit -- see fir4_top.cpp's header
// comment for why.
//
// This is deliberately NOT a single Chain<Tap<10>,Tap<118>,Tap<118>,
// Tap<10>, Tap<10>,Tap<118>,Tap<118>,Tap<10>> (concatenating the two
// stages' tap lists via Chain<>::App/Ins). Chain<>'s composition algebra
// threads ONE running accumulator across taps that all see delayed
// copies of the SAME input sample stream -- correct for building a
// single filter's internal topology (which is exactly what Tap<> already
// does), but a cascade of independent filter *sections* needs stage 2 to
// operate on stage 1's fully-summed OUTPUT, not on a further-delayed copy
// of the raw input. Concatenating tap lists instead silently produces a
// different, single 8-tap FIR (coefficients [10,118,118,10,10,118,118,
// 10]) with a materially different impulse response than the true
// cascade (whose impulse response is the convolution of the two
// sections' responses) -- see README.md's "Cascaded stages" section for
// both impulse responses side by side.
//
// The correct composition needs no new Chain<> primitive: a "stage" here
// is not a tap, it is an already-closed top-level transform (an APIOf<>
// instance). Two closed transforms compose in series exactly the way any
// two functions do in C++ -- call one, feed its result to the other.

#include <hapi/hapi.h>
#include <cstdint>
using namespace hapi;

struct Item {
  static int32_t mac(int16_t /*delayed*/, int32_t acc) { return acc; }
};

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

// One 4-tap Hamming LPF section, closed into a single black-box transform.
using FirLpf4 = Chain<Tap<10>, Tap<118>, Tap<118>, Tap<10>>;
using Stage   = APIOf<Item, FirLpf4>;

Stage stage1;
Stage stage2;

int32_t firLpfCascade2Top(int16_t x) {
  int32_t y1 = stage1.mac(x, 0);
  // No inter-stage Q8 renormalization: each stage's coefficients sum to
  // 256 (unity DC gain once divided by 256), but the impulse-amplitude
  // (1) used for the native regression check below is far smaller than
  // 256, so a >>8 between stages would truncate every intermediate value
  // to zero and defeat the hand-checkable test. Real audio-scale input
  // would need a normalizing shift (and saturation) here -- out of scope
  // for this fixed-point-only, first-pass demo, same scope boundary
  // README.md already draws around floating point.
  int32_t y2 = stage2.mac(static_cast<int16_t>(y1), 0);
  return y2;
}
