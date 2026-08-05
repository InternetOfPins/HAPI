// HLS synthesis target: 4-tap FIR, Chain<Tap<1>,Tap<3>,Tap<3>,Tap<1>>.
// Isolated in its own translation unit -- not part of src/, so
// PlatformIO's native build doesn't compile it, and Bambu only ever sees
// one top-level global object (see OneParse's examples/hls_smoke for the
// same isolation rationale: two same-shaped globals in one TU pull each
// other's static-init machinery into the synthesized footprint).

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

using Fir4 = Chain<Tap<1>, Tap<3>, Tap<3>, Tap<1>>;
using Top  = APIOf<Item, Fir4>;

Top fir4;

int32_t firTop(int16_t x) {
  return fir4.mac(x, 0);
}
