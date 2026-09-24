#include "harness.h"
#include "roll_common.h"
volatile uint8_t x[60]; volatile uint8_t out_;
__attribute__((noinline)) bool cls(wave::Features<60>& f){ return Sparse::proc<0>(f); }
int main(){ uinit(); wave::Features<60> f; for(int i=0;i<60;i++) f.v[i]=x[i]^(i*37);
  MEASURE("rs:", out_=cls(f)); done(); }
