#include "harness.h"
#include "net.h"
volatile uint8_t in_[4]={10,200,30,99}; volatile uint8_t out_;
int main(){ uinit(); BanknoteState f=banknote({in_[0],in_[1],in_[2],in_[3]});
  MEASURE("wave4:", {BanknoteIn& s=hapi::slot<BanknoteTag>(f); uint8_t a=s.variance,b=s.skewness,c=s.curtosis,d=s.entropy; KEEP(a);KEEP(b);KEEP(c);KEEP(d); s.variance=a;s.skewness=b;s.curtosis=c;s.entropy=d; bool r=BanknoteNet::proc(f); KEEP(r); out_=r;}); done(); }
