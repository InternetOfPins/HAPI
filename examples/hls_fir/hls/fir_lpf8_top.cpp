// HLS synthesis target: 8-tap FIR, same Chain<Tap<...>> shape as
// fir8_top.cpp, but real Hamming-windowed-sinc low-pass coefficients
// (fc = 1000 Hz, fs = 8000 Hz, Q8 fixed-point, unity DC gain) instead of
// the fir4_top.cpp/fir8_top.cpp binomial placeholders -- see
// fir_lpf4_top.cpp for the 4-tap version of the same filter design.
// Isolated in its own translation unit -- see fir4_top.cpp's header
// comment for why.

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

// Hamming-windowed sinc LPF, fc=1000Hz/fs=8000Hz, Q8 (x256), sum=256.
using FirLpf8 = Chain<Tap<1>, Tap<10>, Tap<41>, Tap<76>, Tap<76>, Tap<41>, Tap<10>, Tap<1>>;
using Top     = APIOf<Item, FirLpf8>;

Top firLpf8;

int32_t firLpf8Top(int16_t x) {
  return firLpf8.mac(x, 0);
}
