// Native (plain g++, no Bambu) correctness for Reg<T>-based components,
// linked directly against the real hls/*.cpp targets (not a
// reimplementation) -- known-good values are hand-computed, not guessed
// after the fact. Companion to tests/ac_types_native_test.cpp (round 2,
// plain ac_int/ac_fixed correctness, no HAPI composition involved).
#include <cstdint>
#include <cassert>
#include <cstdio>

extern int32_t firLpf4ActypesTop(int16_t);     // examples/hls_fir baseline: raw member
extern int32_t firLpf4ActypesRegTop(int16_t);  // this round: Reg<Sample> shell
extern int32_t acAccumulatorTop(int8_t);
extern void cmacTop(int16_t xr, int16_t xi, int32_t* out_re, int32_t* out_im);

static void fir_raw_vs_shell_test() {
  int16_t impulse[] = {1, 0, 0, 0, 0, 0, 0, 0};
  const int32_t expected[] = {0, 10, 118, 118, 10, 0, 0, 0};
  for (size_t i = 0; i < 8; ++i) {
    int32_t raw   = firLpf4ActypesTop(impulse[i]);
    int32_t shell = firLpf4ActypesRegTop(impulse[i]);
    assert(raw == expected[i]);
    assert(shell == expected[i]);
  }
  printf("fir_raw_vs_shell_test: PASS (both match %s)\n",
         "0 10 118 118 10 0 0 0");
}

static void ac_accumulator_test() {
  // +50 three times, wrapping in signed 8-bit range at the third step:
  // 50, 100, 150 -> 150-256 = -106.
  int32_t a = acAccumulatorTop(50);
  int32_t b = acAccumulatorTop(50);
  int32_t c = acAccumulatorTop(50);
  assert(a == 50);
  assert(b == 100);
  assert(c == -106);
  printf("ac_accumulator_test: PASS\n");
}

static void ac_complex_cmac_test() {
  // coefficient 2-1j, input 3+4j: (3+4j)(2-1j) = (6+4) + (-3+8)j = 10+5j.
  // Second call accumulates another 10+5j -> 20+10j.
  int32_t re, im;
  cmacTop(3, 4, &re, &im);
  assert(re == 10 && im == 5);
  cmacTop(3, 4, &re, &im);
  assert(re == 20 && im == 10);
  printf("ac_complex_cmac_test: PASS\n");
}

int main() {
  fir_raw_vs_shell_test();
  ac_accumulator_test();
  ac_complex_cmac_test();
  printf("ALL HAPI REG TESTS PASS\n");
  return 0;
}
