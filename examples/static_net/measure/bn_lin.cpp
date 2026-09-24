#include "harness.h"
#include "waveCell.h"
#include "linCell.h"
using L=lin::Cell<-300,lin::Sign,lin::In<0,37>,lin::In<1,-91>,lin::In<2,12>,lin::In<3,-5>>;
using N=snet::Net<L>;
volatile uint8_t in_[4]={10,200,30,99}; volatile uint8_t out_;
int main(){ uinit(); wave::Features<4> f{{in_[0],in_[1],in_[2],in_[3]}};
  MEASURE("lin4:", out_=N::proc<0>(f)); done(); }
