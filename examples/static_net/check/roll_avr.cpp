// roll_common.h nets on AVR: build with -DUSE=Unrolled|Rolled|Sparse (60 terms; unrolled T terms, one Roll, one Roll in reversed order)
#include "roll_common.h"
volatile uint8_t in_[60]; volatile uint8_t out_;
#ifndef USE
#define USE Rolled
#endif
int main(){ for(;;){ wave::Features<60> f; for(int i=0;i<60;i++) f.v[i]=in_[i]; out_=USE::proc<0>(f);} }
