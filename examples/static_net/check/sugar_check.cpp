#include "waveCell.h"
#include "inputs.h"
#include "sugar.h"
#include <cstdio>
using namespace sugar;
constexpr auto a=x<inp::A>;
constexpr auto b=x<inp::B>;
constexpr auto orr  = cell<-1>(sign, w<2>*a,  w<2>*b);
constexpr auto nand = cell< 3>(sign, w<-2>*a, w<-2>*b);
constexpr auto xr   = cell<-3>(sign, w<2>*ref(orr), w<2>*ref(nand));
using Net = decltype(net(orr, nand, xr));

// the same net written by hand, for comparison
using Hand = snet::Net<
  lin::Cell<-1,lin::Sign,lin::In<inp::A,2>,lin::In<inp::B,2>>,
  lin::Cell< 3,lin::Sign,lin::In<inp::A,-2>,lin::In<inp::B,-2>>,
  lin::Cell<-3,lin::Sign,lin::RefIn<0,2>,lin::RefIn<1,2>>>;

int main(){ int ok=1;
  for(int p=0;p<2;p++)for(int q=0;q<2;q++){ auto f=inp::ab({uint8_t(p),uint8_t(q)});
    ok&=Net::proc<2>(f)==(p^q) && Hand::proc<2>(f)==(p^q); }
  printf("sizeof=%zu %s\n",sizeof(Net),ok?"ALL OK":"FAIL"); return !ok; }
