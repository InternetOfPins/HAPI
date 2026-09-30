#include "waveCell.h"
#include "inputs.h"
#include "linCell.h"
using Mixed=snet::Net<
  wave::Cell<0,  wave::Threshold, wave::Wave<inp::A,0,6,0,0xff>, wave::Wave<inp::B,0,6,0,0xff>>,
  lin::Cell<-1,  lin::Sign, lin::In<inp::A,2>, lin::In<inp::B,2>>,
  wave::Cell<128,wave::Threshold, wave::RefWave<0,0,6,0,0xff>, wave::RefWave<1,0,6,0,0xff>>,
  lin::Cell<0,   lin::RefIn<2,10>, lin::RefIn<0,-3>, lin::In<inp::A,7>>
>;
volatile uint8_t in_[2]; volatile int16_t out_;
int main(){ for(;;){ auto f=inp::ab({in_[0],in_[1]}); out_=Mixed::proc<3>(f);} }
