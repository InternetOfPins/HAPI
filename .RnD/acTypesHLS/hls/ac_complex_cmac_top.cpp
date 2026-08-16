// ac_complex<ac_fixed<...>>-based complex multiply-accumulate -- proves
// Reg<T> handles an AC-internally-composed type (ac_complex is a plain
// struct of two same-typed fields) exactly like an atomic one, with zero
// extra HAPI code. Zero-fraction-bit CFixed/CAccum (I=W, matching
// fir_lpf4_actypes_top.cpp's Sample/Accum convention) so raw int16_t
// samples map onto them directly with no scaling/overflow to reason
// about -- the point here is proving the shell composes with a
// genuinely-composed AC type, not re-proving real-fraction-bit behavior
// (already covered by tests/ac_types_native_test.cpp).
#include <ac_complex.h>
#include <ac_fixed.h>
#include "../ac_upstream_guard.h"
#include <hapi/hapi.h>
#include <cstdint>
using namespace hapi;

using CFixed  = ac_fixed<16, 16, true>;
using CAccum  = ac_complex<ac_fixed<32, 32, true>>;
using CSample = ac_complex<CFixed>;

template<int16_t CoeffRe, int16_t CoeffIm>
struct CmacOp {
  template<typename I>
  struct Part : I {
    using Base = I;
    using Base::Base;

    CAccum step(CSample x) {
      CAccum coeff{CFixed(CoeffRe), CFixed(CoeffIm)};
      Base::value += CAccum(x) * coeff;
      return Base::value;
    }
  };
};

using ComplexMac = Chain<CmacOp<2, -1>, Reg<CAccum>>;  // coefficient 2 - 1j
using Top        = APIOf<Nil, ComplexMac>;

Top cmac;

// Single top function, one .step() call, both parts out via pointers --
// two independently-callable top-fns each doing their own .step() would
// silently double-advance the shared accumulator if ever called together.
void cmacTop(int16_t xr, int16_t xi, int32_t* out_re, int32_t* out_im) {
  CAccum r = cmac.step(CSample(CFixed(xr), CFixed(xi)));
  *out_re = r.r().to_ac_int().to_int();
  *out_im = r.i().to_ac_int().to_int();
}
