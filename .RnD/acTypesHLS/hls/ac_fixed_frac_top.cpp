// HLS synthesis target: ac_fixed with real fraction bits plus explicit
// quantization/overflow modes (AC_RND, AC_SAT) -- the fraction-bit and
// rounding/saturation machinery fir_lpf4_actypes_top.cpp's zero-fraction-
// bit swap never touched. Runtime input port so nothing constant-folds
// away before scheduling.
#include <ac_fixed.h>
#include "../ac_upstream_guard.h"
#include <cstdint>

// Q4.4: 4 integer bits, 4 fraction bits, round-to-nearest, saturate.
using Frac = ac_fixed<8, 4, true, AC_RND, AC_SAT>;

int32_t acFixedFracTop(int8_t raw) {
  Frac x = Frac(raw) >> 2;      // arithmetic right shift into fraction range
  Frac scaled = x * Frac(1.5);  // real fractional multiply
  return scaled.to_ac_int().to_int();
}
