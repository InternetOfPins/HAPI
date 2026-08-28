// The HAPI Chain<> and the TFLite-Micro interpreter run the SAME network
// (weights from models/hello_world_float.tflite). Sweep x in [0, 2pi];
// they must agree to float rounding.
//   ./native/build.sh
#include <cstdio>
#include <cmath>
#include "net_a.h"
#include "tflm.h"

int main() {
  tflm::setup();
  const float PI = 3.14159265358979f;
  int fails = 0;
  printf("   x        NetA(HAPI)   TFLM        |diff|\n");
  for (int k = 0; k <= 12; ++k) {
    float x = PI * 2.0f * k / 12.0f;
    float a = net_a::infer(x);
    float t = tflm::infer(x);
    float d = std::fabs(a - t);
    bool ok = d < 1e-4f;
    fails += !ok;
    printf("%6.3f    % .7f   % .7f   %.2e  %s\n", x, a, t, d, ok ? "" : "<-- MISMATCH");
  }
  printf(fails ? "\n%d MISMATCH(es)\n" : "\nHAPI Chain<> AND TFLite-Micro AGREE\n", fails);
  return fails ? 1 : 0;
}
