// HLS synthesis target: 4-tap FIR with RUNTIME (not compile-time-NTTP)
// coefficients -- same Chain<Tap<...>> shape as fir4_top.cpp, but each
// Tap<Idx> now indexes a `volatile` coefficient table instead of using
// Idx as the multiplicand itself. `volatile` forces a genuine memory
// read Bambu cannot constant-fold away, modeling a real, different DSP
// use case: a runtime-configurable/adaptive filter (e.g. a tunable EQ)
// where coefficient values are genuinely unknown at synthesis time.
//
// fir4_top.cpp already proved the composition survives synthesis; this
// file answers a different question -- does a real DSP-mappable
// multiplier get inferred when the coefficient can't be constant-folded?
// (fir4_top.cpp's coefficients -- 1, 3, 3, 1 -- are small enough that
// Bambu's own optimizer strength-reduces every multiply into shift+add,
// so "Estimated number of DSPs: 0" there is Bambu being smart about
// compile-time-known coefficients, not a composition limitation. See
// README.md for the full comparison.)

#include <hapi/hapi.h>
#include <cstdint>
using namespace hapi;

volatile int16_t coeffTable[4] = {1, 3, 3, 1};

struct Item {
  static int32_t mac(int16_t /*delayed*/, int32_t acc) { return acc; }
};

template<int Idx>
struct Tap {
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

using Fir4Rt = Chain<Tap<0>, Tap<1>, Tap<2>, Tap<3>>;
using Top    = APIOf<Item, Fir4Rt>;

Top fir4rt;

int32_t firRtTop(int16_t x) {
  return fir4rt.mac(x, 0);
}
