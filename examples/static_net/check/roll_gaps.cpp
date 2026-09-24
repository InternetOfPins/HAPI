// roll.h beyond roll_host.cpp (which rolls all 60 inputs, in order and reversed): indices with GAPS (only some inputs used, in no
// particular order, one of them twice), weights at both ends of int8, and the dense flag. Compared on the raw int32 sum (no
// readout), so a wrong term cannot hide behind a threshold.
#include "roll_common.h"
#include <cstdio>
#include <cstdlib>
using namespace snet;
static_assert( IsIota<0>::value && IsIota<0,0>::value && IsIota<0,0,1,2>::value, "0..n-1 in order is dense");
static_assert(!IsIota<0,1,0>::value && !IsIota<0,0,2>::value && !IsIota<0,1,2>::value, "reversed, gapped, or not starting at 0 is not");
using U = Net<C<T<3,17>,T<4,-90>,T<10,5>,T<11,127>,T<12,-128>,T<30,60>,T<59,-7>,T<5,9>,T<5,-4>,B32<-1500>>>;
using R = Net<C<Roll<T<3,17>,T<4,-90>,T<10,5>,T<11,127>,T<12,-128>,T<30,60>,T<59,-7>,T<5,9>,T<5,-4>>,B32<-1500>>>;
int main(){ int agree=0, n=20000; srand(11);
  for(int t=0;t<n;t++){ wave::Features<60> f; for(int i=0;i<60;i++) f.v[i]=t<2 ? (t?255:0) : rand()&255;    // rows 0 and 1: all 0, all 255
    agree += U::proc<0>(f)==R::proc<0>(f); }
  printf("gapped rolled==unrolled %d/%d\n",agree,n); return agree!=n; }
