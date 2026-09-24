#include "harness.h"
#include <avr/pgmspace.h>
const int8_t W[60] PROGMEM={-93,18,89,78,68,-111,-62,-97,-1,67,-12,-7,39,-30,74,-74,-103,-3,-120,101,86,-28,-17,28,68,69,-127,51,-13,-59,57,78,-69,24,114,-101,103,-46,-120,-122,-121,39,11,-125,113,98,-30,48,-72,121,-19,58,-120,8,-71,68,-15,113,-1,14};
volatile uint8_t x[60]; volatile uint8_t out_;
__attribute__((noinline)) bool cls(const uint8_t*v){ int32_t a=-2000; for(uint8_t i=0;i<60;i++) a+=int16_t(int16_t((int8_t)pgm_read_byte(&W[i]))*int16_t(v[i])); return a>0; }
int main(){ uinit(); uint8_t v[60]; for(int i=0;i<60;i++) v[i]=x[i]^(i*37);
  MEASURE("table60:", out_=cls(v)); done(); }
