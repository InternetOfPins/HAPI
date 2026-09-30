#include "harness.h"
#include "roll_common.h"
volatile uint8_t x[60]; volatile uint8_t out_;
__attribute__((noinline)) bool cls(inp::Bytes60& f){ return Rolled::proc<0>(f); }
int main(){ uinit(); inp::Bytes60 f; for(int i=0;i<60;i++) inp::band(f)[i]=x[i]^(i*37);
  MEASURE("rr:", out_=cls(f)); done(); }
