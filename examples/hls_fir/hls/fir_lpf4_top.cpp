// HLS synthesis target: 4-tap FIR, same Chain<Tap<...>> shape as
// fir4_top.cpp, but real Hamming-windowed-sinc low-pass coefficients
// (fc = 1000 Hz, fs = 8000 Hz, Q8 fixed-point, unity DC gain) instead of
// the fir4_top.cpp/fir8_top.cpp binomial placeholders. Isolated in its
// own translation unit -- see fir4_top.cpp's header comment for why.

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
using FirLpf4 = Chain<Tap<10>, Tap<118>, Tap<118>, Tap<10>>;
using Top     = APIOf<Item, FirLpf4>;

Top firLpf4;

int32_t firLpf4Top(int16_t x) {
  return firLpf4.mac(x, 0);
}
