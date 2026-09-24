// EXPECT-ERROR: no cell in the net matches Q
// RI<77> names an id no cell carries: one clear message instead of HAPI's internal "no type named 'Result' in FindFirst_<...>".
#include "waveCell.h"
#include "refid.h"
template<size_t j,bool n,int s,wave::u8 p,wave::u8 m> using RW=wave::WaveOf<snet::RefId<j>,n,s,p,m>;
using Bad=snet::Net<wave::Cell<0,hapi::Tag<1>,RW<77,0,0,0,0xff>>>;
int main(){ wave::Features<1> f{{0}}; return Bad::proc<0>(f); }
