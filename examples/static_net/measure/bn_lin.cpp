#include "harness.h"
#include "waveCell.h"
#include "linCell.h"
#include "net.h"
using L=lin::Cell<-300,lin::Sign,lin::In<Variance,37>,lin::In<Skewness,-91>,lin::In<Curtosis,12>,lin::In<Entropy,-5>>;
using N=snet::Net<L>;
volatile uint8_t in_[4]={10,200,30,99}; volatile uint8_t out_;
int main(){ uinit(); BanknoteState f=banknote({in_[0],in_[1],in_[2],in_[3]});
  MEASURE("lin4:", out_=N::proc<0>(f)); done(); }
