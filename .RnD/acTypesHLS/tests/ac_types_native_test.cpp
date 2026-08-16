// Native (plain g++, no Bambu) functional correctness check for HLSLibs
// AC Datatypes -- run against known-good reference values with assert(),
// so a wrong result aborts instead of passing quietly. Companion to the
// hls/ synthesis-path targets in this same directory, which check that
// Bambu accepts the same types/operators, not that the arithmetic is
// correct (no RTL simulation harness exists for that -- see HANDOFF.md).
#include <ac_int.h>
#include <ac_fixed.h>
#include "../ac_upstream_guard.h"
#include <cassert>
#include <cstdio>

static void ac_int_tests() {
  // Unsigned wraparound at the declared bit width.
  ac_int<4, false> u = 15;
  u = u + 1;
  assert(u.to_int() == 0);

  // Signed range: ac_int<4,true> is -8..7, wraps like a normal 4-bit
  // two's-complement int on overflow (ac_int has no O_mode/saturation --
  // that's ac_fixed-only, checked below).
  ac_int<4, true> s = 7;
  s = s + 1;
  assert(s.to_int() == -8);

  // Single-bit types: easy off-by-one case, unsigned vs signed range.
  ac_int<1, false> b0 = 1;
  assert(b0.to_int() == 1);
  ac_int<1, true> b1 = -1;
  assert(b1.to_int() == -1);

  // Result-width auto-growth: adding two 8-bit signed values that would
  // overflow an 8-bit sum must not overflow the *result* type.
  ac_int<8, true> a8 = 100, c8 = 100;
  auto sum = a8 + c8;                 // rt<> widens the result type
  assert(sum.to_int() == 200);

  // Multiplication similarly must not overflow: 100*100=10000 doesn't
  // fit in 8 or even 9 bits, but the multiply result type does.
  auto prod = a8 * c8;
  assert(prod.to_int() == 10000);

  // Static bit-slice read: upper nibble of 0xA5 is 0xA.
  ac_int<8, false> v = 0xA5;
  ac_int<4, false> hi = v.slc<4>(4);
  assert(hi.to_int() == 0xA);

  printf("ac_int_tests: PASS\n");
}

static void ac_fixed_tests() {
  // Real fractional representation: ac_fixed<8,4,true> is 4 integer + 4
  // fraction bits -- unlike hls_fir's ac_fixed<W,W,true> (zero fraction
  // bits), this exercises the part the FIR check never touched.
  ac_fixed<8, 4, true> half = 2.5;
  assert(half.to_double() == 2.5);

  ac_fixed<8, 4, true> neg = -1.25;
  assert(neg.to_double() == -1.25);

  // Truncation (AC_TRN, the default) vs rounding (AC_RND) on a value
  // that isn't exactly representable at 4 fraction bits (resolution
  // 1/16 = 0.0625): 1.05 sits between 1.0 and 1.0625.
  ac_fixed<8, 4, true, AC_TRN> trn = 1.05;
  ac_fixed<8, 4, true, AC_RND> rnd = 1.05;
  assert(trn.to_double() == 1.0);      // truncates down
  assert(rnd.to_double() == 1.0625);   // rounds to nearest representable

  // Saturation (AC_SAT) vs wraparound (AC_WRAP, the default) on
  // out-of-range assignment: ac_fixed<4,4,true> range is -8..7 (0
  // fraction bits here so the numbers stay simple), assigning 10.0 is
  // out of range either way.
  ac_fixed<4, 4, true, AC_TRN, AC_WRAP> wrapped = 10.0;
  ac_fixed<4, 4, true, AC_TRN, AC_SAT>  saturated = 10.0;
  assert(wrapped.to_double() == -6.0);  // 10 wraps mod 16 into range
  assert(saturated.to_double() == 7.0); // clamps to the format's max

  printf("ac_fixed_tests: PASS\n");
}

int main() {
  ac_int_tests();
  ac_fixed_tests();
  printf("ALL NATIVE AC_TYPES TESTS PASS\n");
  return 0;
}
