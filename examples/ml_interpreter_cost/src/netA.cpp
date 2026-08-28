// HAPI Chain<> running the hello_world network.
#include <stdint.h>
#include "../net_a.h"

volatile float v_x = 1.5f;
volatile float g_out;

static void run_once() { g_out = net_a::infer(v_x); }

#if defined(ARDUINO)
#include <Arduino.h>
void setup() {
#ifdef HW_READBACK
  Serial.begin(115200);
#endif
  run_once();
}
void loop() {
#ifdef HW_READBACK
  Serial.printf("netA y=%.5f\n", (double)g_out);
  delay(1000);
#endif
}
#else
int main() { run_once(); for (;;) {} }   // bare-metal: don't return
#endif
