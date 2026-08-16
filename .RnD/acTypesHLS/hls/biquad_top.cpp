// HLS synthesis target: a single 2nd-order IIR (biquad) section, direct
// form I -- y[n] = b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2] --
// genuinely different from every prior hls_fir target: a feedforward
// delay line (same shape as hls_fir's Tap/Reg<Sample> pattern) PLUS a
// feedback delay line whose state can only be updated once the output is
// known, not inline with the sum like a feedforward tap. Both delay lines
// use the same Reg<T> shell (HAPI/include/hapi/reg.h) -- one primitive,
// two different roles.
//
// Coefficients: b1=0.5, b2=0.25, -a1=0.5, -a2=-0.25 (all exact powers of
// two, chosen so the impulse response is exactly hand-computable with no
// rounding, and so the section is genuinely stable/decaying, unlike a
// zero-fraction-bit coefficient which can only ever have magnitude >=1).
// Coefficients are Q8.8 raw bit patterns (rawCoeff<>), not the ac_fixed
// double constructor: empirically confirmed a `static const Sample coeff
// = 0.5;` pattern synthesizes a real stateful lazy-init guard (an actual
// runtime branch + register) under Bambu instead of folding to a
// constant, and a template-parameterized free function is not stable to
// rely on for folding either -- set_slc-based raw bits fold cleanly with
// neither that guard nor any floating-point hardware (verified directly).
#include <hapi/hapi.h>
#include <ac_fixed.h>
#include <ac_int.h>
#include <cstdint>
using namespace hapi;

#if AC_VERSION < 4
#error "resolved to bambu's bundled ac_types fork, not real upstream github.com/hlslibs/ac_types"
#endif

using Sample = ac_fixed<16, 8, true>;   // Q8.8
using Accum  = ac_fixed<32, 16, true>;  // Q16.16

template<int16_t RawBits>
static inline Sample rawCoeff() {
  Sample c;
  c.set_slc(0, ac_int<16, true>(RawBits));
  return c;
}

struct Item {
  static Accum mac(Sample /*delayed*/, Accum acc) { return acc; }
  static Accum fbSum(Accum acc) { return acc; }
  static void  fbPush(Sample /*newVal*/) {}
};

// Feedforward tap: same shape as hls_fir's proven Tap/TapLogic -- reads
// and updates its own delay in one pass, forwards the running sum.
template<int16_t RawBits>
struct FFTapLogic {
  template<typename I>
  struct Part : I {
    using Base = I;
    using Base::Base;
    Accum mac(Sample x, Accum acc) {
      Accum sum   = acc + Accum(rawCoeff<RawBits>()) * Accum(Base::value);
      Sample prev = Base::value;
      Base::value = x;
      return I::mac(prev, sum);
    }
  };
};
template<int16_t RawBits>
using FFTap = Chain<FFTapLogic<RawBits>, Reg<Sample>>;

// Feedback tap: contributes to the sum via fbSum() using its OLD delayed
// output, but its delay line only shifts via fbPush(), called once the
// final output is known -- unlike a feedforward tap, it cannot update
// itself inline with the sum pass, since the value it needs to store
// (the output) doesn't exist yet at that point.
template<int16_t RawBits>
struct FBTapLogic {
  template<typename I>
  struct Part : I {
    using Base = I;
    using Base::Base;
    Accum fbSum(Accum acc) {
      return I::fbSum(acc + Accum(rawCoeff<RawBits>()) * Accum(Base::value));
    }
    void fbPush(Sample newVal) {
      Sample prev = Base::value;
      Base::value = newVal;
      I::fbPush(prev);
    }
  };
};
template<int16_t RawBits>
using FBTap = Chain<FBTapLogic<RawBits>, Reg<Sample>>;

// b1=0.5(128), b2=0.25(64); -a1=0.5(128), -a2=-0.25(-64).
using Biquad = Chain<FFTap<128>, FFTap<64>, FBTap<128>, FBTap<-64>>;
using Top    = APIOf<Item, Biquad>;

Top biquad;

Sample biquadStep(Sample x) {
  Accum ff = biquad.mac(x, Accum(0));
  Accum fb = biquad.fbSum(Accum(0));
  Sample y = Accum(ff + fb);  // narrow Q16.16 -> Q8.8, quantized correctly
  biquad.fbPush(y);
  return y;
}

// Raw Q8.8 bits out (e.g. 0.5 -> 128) -- no floating-point conversion in
// the synthesized path; scaling back to a real value is the test
// harness's job, not this function's.
int32_t biquadTop(int16_t xr) {
  return biquadStep(Sample(xr)).template slc<16>(0).to_int();
}
