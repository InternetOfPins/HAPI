// HLS synthesis target: ac_int arithmetic that a hand-rolled fixed-width
// int can't express directly -- auto-widening add/multiply plus a static
// bit-slice read -- exercising ac_int's own operator/rt<> machinery under
// Bambu, not just ac_fixed (see fir_lpf4_actypes_top.cpp for that one).
// Runtime input port so nothing constant-folds away before scheduling.
#include <ac_int.h>
#include "../ac_upstream_guard.h"
#include <cstdint>

int32_t acIntOpsTop(int8_t raw) {
  ac_int<8, true> x = raw;
  auto sq   = x * x;            // auto-widened product, no overflow
  auto sum  = sq + x;           // auto-widened sum
  ac_int<8, false> u = static_cast<uint8_t>(raw);
  ac_int<4, false> hi = u.slc<4>(4);   // static bit-slice read
  return sum.to_int() + hi.to_int();
}
