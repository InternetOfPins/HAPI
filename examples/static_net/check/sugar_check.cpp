#include "waveCell.h"
#include "sugar.h"
#include <cstdio>
using namespace sugar;
constexpr auto a=x<0>;
constexpr auto b=x<1>;
constexpr auto orr  = cell<-1>(sign, w<2>*a,  w<2>*b);
constexpr auto nand = cell< 3>(sign, w<-2>*a, w<-2>*b);
constexpr auto xr   = cell<-3>(sign, w<2>*ref(orr), w<2>*ref(nand));
using Net = decltype(net(orr, nand, xr));

// the same net written by hand, for comparison
using Hand = snet::Net<
  lin::Cell<-1,lin::Sign,lin::In<0,2>,lin::In<1,2>>,
  lin::Cell< 3,lin::Sign,lin::In<0,-2>,lin::In<1,-2>>,
  lin::Cell<-3,lin::Sign,lin::RefIn<0,2>,lin::RefIn<1,2>>>;

int main(){ int ok=1;
  for(int p=0;p<2;p++)for(int q=0;q<2;q++){ wave::Features<2> f{{(wave::u8)p,(wave::u8)q}};
    ok&=Net::proc<2>(f)==(p^q) && Hand::proc<2>(f)==(p^q); }
  printf("sizeof=%zu %s\n",sizeof(Net),ok?"ALL OK":"FAIL"); return !ok; }
