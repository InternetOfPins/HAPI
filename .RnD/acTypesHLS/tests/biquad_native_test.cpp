// Native (plain g++, no Bambu) correctness for the biquad IIR targets,
// linked directly against the real hls/*.cpp targets (not a
// reimplementation). Known-good values are hand-derived from the
// difference equation y[n]=b1*x[n-1]+b2*x[n-2]-a1*y[n-1]-a2*y[n-2] with
// b1=0.5, b2=0.25, -a1=0.5, -a2=-0.25 (all exact powers of two, so the
// impulse response has no rounding to account for), and the cascade's
// expected sequence is the convolution of the single section's impulse
// response with itself (same cross-check methodology as
// examples/hls_fir/hls/fir_lpf_cascade2_top.cpp).
#include <cstdint>
#include <cassert>
#include <cstdio>

extern int32_t biquadTop(int16_t);
extern int32_t biquadCascade2Top(int16_t);

static void biquad_single_test() {
  int16_t impulse[] = {1, 0, 0, 0, 0, 0, 0, 0};
  const int32_t expected[] = {0, 128, 128, 32, -16, -16, -4, 2};  // raw Q8.8
  for (size_t i = 0; i < 8; ++i) {
    assert(biquadTop(impulse[i]) == expected[i]);
  }
  printf("biquad_single_test: PASS (0 128 128 32 -16 -16 -4 2)\n");
}

static void biquad_cascade_test() {
  int16_t impulse[] = {1, 0, 0, 0, 0, 0, 0, 0};
  const int32_t expected[] = {0, 0, 64, 128, 96, 16, -28, -24};  // raw Q8.8
  for (size_t i = 0; i < 8; ++i) {
    assert(biquadCascade2Top(impulse[i]) == expected[i]);
  }
  printf("biquad_cascade_test: PASS (0 0 64 128 96 16 -28 -24)\n");
}

int main() {
  biquad_single_test();
  biquad_cascade_test();
  printf("ALL BIQUAD TESTS PASS\n");
  return 0;
}
