// cells in a different order than refid_check.cpp: ids non-monotonic and out of step with indices,
// an unrelated cell first shifts every index, OR sits before NAND -- wiring is by id, not position
#include "waveCell.h"
#include "linCell.h"
#include "refid.h"
#include <cstdio>
enum : int { NAND=99, OR=7, XOR=42, RAW=5, PAD=1 };
template<size_t j,bool n,int s,wave::u8 p,wave::u8 m> using RW=wave::WaveOf<snet::RefId<j>,n,s,p,m>;
template<int id,lin::acc w> using RI=lin::Term<snet::RefId<id>,w>;
using P=snet::Net<
  lin::Cell<5,  hapi::Tag<PAD>,  lin::In<0,1>>,                                                  // idx0, unrelated
  lin::Cell<-1, hapi::Tag<OR>,   lin::Sign, lin::In<0,2>, lin::In<1,2>>,                          // idx1 (OR before NAND)
  wave::Cell<0, hapi::Tag<NAND>, wave::Threshold, wave::Wave<0,0,6,0,0xff>, wave::Wave<1,0,6,0,0xff>>, // idx2
  wave::Cell<128,hapi::Tag<XOR>, wave::Threshold, RW<NAND,0,6,0,0xff>, RW<OR,0,6,0,0xff>>,        // idx3
  lin::Cell<0,  hapi::Tag<RAW>,  RI<XOR,10>, RI<NAND,-3>, lin::In<0,7>>                          // idx4
>;
int main(){ int ok=1;
  for(int a=0;a<2;a++)for(int b=0;b<2;b++){ wave::Features<2> f{{(wave::u8)a,(wave::u8)b}};
    int x=P::proc<3>(f), r=P::proc<4>(f), rexp=10*(a^b)-3*!(a&&b)+7*a; ok&=x==(a^b)&&r==rexp; }
  printf("permuted %s\n",ok?"ALL OK":"FAIL"); return !ok; }
