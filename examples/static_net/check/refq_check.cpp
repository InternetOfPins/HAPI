// RefQ at run time: wiring by a criterion, not an index. The last two cases pin what an id is: just a query, first match wins.
#include "waveCell.h"
#include "inputs.h"
#include "linCell.h"
#include "refid.h"
#include <cstdio>
enum : int { NAND=10, OR=20, XOR=30, DUP=5, VIA=6 };
template<typename Q,bool n,int s,wave::u8 p,wave::u8 m> using RQ=wave::WaveOf<snet::RefQ<Q>,n,s,p,m>;
template<int id,lin::acc w> using RI=lin::Term<snet::RefId<id>,w>;
template<typename Q> using Tag_=hapi::SameAs<Q>;
// XOR wires NAND by its tag and OR as "the cell that has lin::Sign": a criterion that is not an id
using X=snet::Net<
  wave::Cell<0,  hapi::Tag<NAND>, wave::Threshold, wave::Wave<inp::A,0,6,0,0xff>, wave::Wave<inp::B,0,6,0,0xff>>,
  lin::Cell<-1,  hapi::Tag<OR>,   lin::Sign, lin::In<inp::A,2>, lin::In<inp::B,2>>,
  wave::Cell<128,hapi::Tag<XOR>,  wave::Threshold, RQ<hapi::SameAs<hapi::Tag<NAND>>,0,6,0,0xff>, RQ<hapi::SameAs<lin::Sign>,0,6,0,0xff>>
>;
// two cells carry Tag<DUP>: the first outputs a, the second b. Wiring by that tag gets the FIRST (a), not an error.
using Dup=snet::Net<
  lin::Cell<0, hapi::Tag<DUP>, lin::In<inp::A,1>>,
  lin::Cell<0, hapi::Tag<DUP>, lin::In<inp::B,1>>,
  lin::Cell<0, hapi::Tag<VIA>, RI<DUP,1>>
>;
static_assert(snet::CellsMatching<hapi::SameAs<hapi::Tag<DUP>>,Dup>::size==2,"the net really has two DUP cells");
int main(){ int ok=1;
  for(int a=0;a<2;a++)for(int b=0;b<2;b++){ auto f=inp::ab({uint8_t(a),uint8_t(b)});
    ok&=X::proc<2>(f)==(a^b);              // criterion-wired XOR
    ok&=Dup::proc<2>(f)==a;                // first match wins: a, whereas the second DUP cell would give b
  }
  printf("refq %s\n",ok?"ALL OK":"FAIL"); return !ok; }
