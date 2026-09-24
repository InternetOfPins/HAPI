// The running example of static_net: a 4-input banknote classifier, written as a static net.
//
// BanknoteNet (models/banknote/net.h) is a TYPE: a HAPI composition of `wave` components, its trained parameters
// template arguments. Nothing is instantiated and there is no weight table, no state, no interpreter: the whole
// classifier compiles to a handful of shifts, masks and adds. On an ATmega328p that is 44 bytes of flash and 25 cycles
// per row (README, "What was measured").
//
// This program feeds the 274 held-out rows of the fold it was trained on through it and checks two things: agreement
// with the C reference model that trained it (the column `expected_model_out` of wave_vectors.h) and accuracy against
// the labels. It runs unchanged on the host and on an AVR (rows in flash, report over Serial).
#ifdef ARDUINO
  #include <Arduino.h>
  #define WAVE_VEC_ATTR PROGMEM                                            // the rows live in flash
  #define ROW(r,j) pgm_read_byte(&WAVE_VEC[r][j])
#else
  #include <cstdio>
  #define WAVE_VEC_ATTR
  #define ROW(r,j) (WAVE_VEC[r][j])
#endif
#include "net.h"
#include "wave_vectors.h"

struct Report { int agree, correct; };

static Report classify() {
  Report rep{0, 0};
  for (int r = 0; r < WAVE_NVEC; r++) {
    wave::Features<4> f{{ROW(r,0), ROW(r,1), ROW(r,2), ROW(r,3)}};       // the state: four input bytes, the only runtime data
    const bool y = BanknoteNet::proc(f);                                    // the net, evaluated
    rep.agree   += (y == bool(ROW(r,5)));                                   // the reference model's output for this row
    rep.correct += (y == bool(ROW(r,4)));                                   // the row's label
  }
  return rep;
}

#ifdef ARDUINO
void setup() { Serial.begin(115200); }
void loop() {
  const Report r = classify();
  Serial.print(F("static_net banknote: agree ")); Serial.print(r.agree); Serial.print('/'); Serial.print(WAVE_NVEC);
  Serial.print(F(", correct ")); Serial.print(r.correct); Serial.print('/'); Serial.println(WAVE_NVEC);
  delay(2000);
}
#else
int main() {
  const Report r = classify();
  printf("static_net banknote: agree %d/%d, correct %d/%d\n", r.agree, WAVE_NVEC, r.correct, WAVE_NVEC);
  return r.agree != WAVE_NVEC;
}
#endif
