#include "harness.h"
#include "net.h"
volatile uint8_t in_[4]={10,200,30,99}; volatile uint8_t out_;
int main(){ uinit(); wave::Features<4> f{{in_[0],in_[1],in_[2],in_[3]}};
  MEASURE("wave4:", {uint8_t a=f.v[0],b=f.v[1],c=f.v[2],d=f.v[3]; KEEP(a);KEEP(b);KEEP(c);KEEP(d); f.v[0]=a;f.v[1]=b;f.v[2]=c;f.v[3]=d; bool r=BanknoteNet::proc(f); KEEP(r); out_=r;}); done(); }
