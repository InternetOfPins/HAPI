// the XOR net of sugar_check.cpp written by hand, no sugar: the reference sugar_avr.cpp must equal
#include "waveCell.h"
#include "linCell.h"
using Net = snet::Net<
  lin::Cell<-1,lin::Sign,lin::In<0,2>,lin::In<1,2>>,
  lin::Cell< 3,lin::Sign,lin::In<0,-2>,lin::In<1,-2>>,
  lin::Cell<-3,lin::Sign,lin::RefIn<0,2>,lin::RefIn<1,2>>>;
volatile uint8_t in_[2]; volatile int16_t out_;
int main(){ for(;;){ wave::Features<2> f{{in_[0],in_[1]}}; out_=Net::proc<2>(f);} }
