// HLS synthesis target: 8-tap FIR, Chain<Tap<1>,Tap<7>,Tap<21>,Tap<35>,
// Tap<35>,Tap<21>,Tap<7>,Tap<1>>. Isolated in its own translation unit --
// see fir4_top.cpp's header comment for why.

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

using Fir8 = Chain<Tap<1>, Tap<7>, Tap<21>, Tap<35>, Tap<35>, Tap<21>, Tap<7>, Tap<1>>;
using Top  = APIOf<Item, Fir8>;

Top fir8;

int32_t firTop(int16_t x) {
  return fir8.mac(x, 0);
}
