// Same 4-tap Hamming-LPF FIR, coefficients and ac_fixed types as
// examples/hls_fir/hls/fir_lpf4_actypes_top.cpp, but the delay register
// is split out into a reusable Reg<Sample> shell instead of a raw member
// -- proves the shell composes cleanly (via HAPI's mono_block: a
// Chain<TapLogic,Reg<Sample>> used as one element of the outer Chain) and
// costs nothing extra under Bambu vs. the raw-member baseline.
#include <hapi/hapi.h>
#include <ac_fixed.h>
#include <cstdint>
using namespace hapi;

#if AC_VERSION < 4
#error "resolved to bambu's bundled ac_types fork, not real upstream github.com/hlslibs/ac_types"
#endif

using Sample = ac_fixed<16, 16, true>;
using Accum  = ac_fixed<32, 32, true>;

struct Item {
  static Accum mac(Sample /*delayed*/, Accum acc) { return acc; }
};

template<int16_t Coeff>
struct TapLogic {
  template<typename I>
  struct Part : I {
    using Base = I;
    using Base::Base;

    Accum mac(Sample x, Accum acc) {
      Accum sum   = acc + Accum(Coeff) * Accum(Base::value);
      Sample prev = Base::value;
      Base::value = x;
      return I::mac(prev, sum);
    }
  };
};

template<int16_t Coeff>
using Tap = Chain<TapLogic<Coeff>, Reg<Sample>>;

using FirLpf4AcReg = Chain<Tap<10>, Tap<118>, Tap<118>, Tap<10>>;
using Top          = APIOf<Item, FirLpf4AcReg>;

Top firLpf4AcReg;

int32_t firLpf4ActypesRegTop(int16_t x) {
  return firLpf4AcReg.mac(Sample(x), Accum(0)).to_int();
}
