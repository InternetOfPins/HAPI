// HLS synthesis target: same 4-tap Hamming-LPF FIR shape and coefficients
// as fir_lpf4_top.cpp, but the delay register/accumulator type is swapped
// from the hand-rolled int16_t/int32_t to HLSLibs AC Datatypes'
// ac_fixed<W,I,true> (https://github.com/hlslibs/ac_types) -- a Bambu HLS
// compatibility check for that third-party bit-accurate type, not a new
// filter design. I is deliberately set equal to W (zero fraction bits) so
// this stays a pure type-family swap -- exercising ac_fixed's constructors/
// operators in the datapath -- rather than introducing new fixed-point
// scaling semantics; values and impulse response are bit-identical to
// fir_lpf4_top.cpp's plain-int version.

#include <hapi/hapi.h>
#include <ac_fixed.h>
#include <cstdint>
using namespace hapi;

// Bambu bundles its own older (Mentor Graphics 3.7.2, 2017, AC_VERSION 3)
// ac_types fork on its default include path; a wrong -I search order
// would silently resolve to that instead of the real upstream header
// (AC_VERSION 4) this file exists to test. Fail loud, not quiet.
#if AC_VERSION < 4
#error "resolved to bambu's bundled ac_types fork, not real upstream github.com/hlslibs/ac_types"
#endif

using Sample = ac_fixed<16, 16, true>;  // drop-in for the hand-rolled int16_t
using Accum  = ac_fixed<32, 32, true>;  // drop-in for the hand-rolled int32_t

struct Item {
  static Accum mac(Sample /*delayed*/, Accum acc) { return acc; }
};

template<int16_t Coeff>
struct Tap {
  template<typename I>
  struct Part : I {
    using Base = I;
    using Base::Base;
    Sample z{0};

    Accum mac(Sample x, Accum acc) {
      Accum sum   = acc + Accum(Coeff) * Accum(z);
      Sample prev = z;
      z = x;
      return I::mac(prev, sum);
    }
  };
};

// Same Hamming-windowed sinc LPF coefficients as fir_lpf4_top.cpp.
using FirLpf4Ac = Chain<Tap<10>, Tap<118>, Tap<118>, Tap<10>>;
using Top       = APIOf<Item, FirLpf4Ac>;

Top firLpf4Ac;

int32_t firLpf4ActypesTop(int16_t x) {
  return firLpf4Ac.mac(Sample(x), Accum(0)).to_int();
}
