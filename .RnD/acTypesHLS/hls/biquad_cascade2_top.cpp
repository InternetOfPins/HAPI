// HLS synthesis target: two independent biquad_top.cpp-equivalent
// sections cascaded in series -- stage 1's output feeds stage 2 as its
// input, same "two already-closed APIOf<> instances, plain function
// composition, not Chain<>-splicing" shape as
// examples/hls_fir/hls/fir_lpf_cascade2_top.cpp (see that file's header
// comment for why concatenating tap lists would be wrong here too).
// Isolated in its own translation unit, same rationale as every other
// target in this investigation.
#include <hapi/hapi.h>
#include <ac_fixed.h>
#include <ac_int.h>
#include <cstdint>
using namespace hapi;

#if AC_VERSION < 4
#error "resolved to bambu's bundled ac_types fork, not real upstream github.com/hlslibs/ac_types"
#endif

using Sample = ac_fixed<16, 8, true>;
using Accum  = ac_fixed<32, 16, true>;

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

using Biquad = Chain<FFTap<128>, FFTap<64>, FBTap<128>, FBTap<-64>>;
using Stage  = APIOf<Item, Biquad>;

Stage stage1;
Stage stage2;

static Sample step(Stage& s, Sample x) {
  Accum ff = s.mac(x, Accum(0));
  Accum fb = s.fbSum(Accum(0));
  Sample y = Accum(ff + fb);
  s.fbPush(y);
  return y;
}

int32_t biquadCascade2Top(int16_t xr) {
  Sample y1 = step(stage1, Sample(xr));
  Sample y2 = step(stage2, y1);
  return y2.template slc<16>(0).to_int();
}
