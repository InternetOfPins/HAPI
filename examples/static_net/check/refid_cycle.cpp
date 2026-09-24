#include "waveCell.h"
#include "refid.h"
template<size_t j,bool n,int s,wave::u8 p,wave::u8 m> using RW=wave::WaveOf<snet::RefId<j>,n,s,p,m>;
using Bad=snet::Net<wave::Cell<0,hapi::Tag<1>,RW<2,0,0,0,0xff>>, wave::Cell<0,hapi::Tag<2>,RW<1,0,0,0,0xff>>>;
int main(){ wave::Features<1> f{{0}}; return Bad::proc<1>(f); }
