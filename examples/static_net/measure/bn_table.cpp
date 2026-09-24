#include "harness.h"
#include <avr/pgmspace.h>
const int8_t W[4] PROGMEM={37,-91,12,-5};
volatile uint8_t in_[4]={10,200,30,99}; volatile uint8_t out_;
__attribute__((noinline)) bool cls(const uint8_t*x){ int16_t a=-300; for(uint8_t i=0;i<4;i++) a+=int16_t((int8_t)pgm_read_byte(&W[i]))*x[i]; return a>0; }
int main(){ uinit(); uint8_t x[4]={in_[0],in_[1],in_[2],in_[3]};
  MEASURE("table4:", out_=cls(x)); done(); }
