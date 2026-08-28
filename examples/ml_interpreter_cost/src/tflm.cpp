// TFLite-Micro MicroInterpreter running the hello_world network.
// -DTFLM_INT8 for the int8 model.
#include <stdint.h>
#include "../tflm.h"

volatile float v_x = 1.5f;
volatile float g_out;

static void run_once() { g_out = tflm::infer(v_x); }

#if defined(ARDUINO)
#include <Arduino.h>
void setup() {
#ifdef HW_READBACK
  Serial.begin(115200);
#endif
  tflm::setup();
  run_once();
}
void loop() {
#ifdef HW_READBACK
  Serial.printf("tflm y=%.5f\n", (double)g_out);
  delay(1000);
#endif
}
#else
int main() { tflm::setup(); run_once(); for (;;) {} }
#endif
