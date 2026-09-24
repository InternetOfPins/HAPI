// the cell of sugar_roll_avr.cpp with the rolled form written out by hand, no sugar: what sugar::cell must pick from 6 terms up
#include "waveCell.h"
#include "linCell.h"
#include "roll.h"
using Net = snet::Net<hapi::APIOf<lin::API,lin::Sign,snet::Roll<snet::T<0,-93>,snet::T<1,18>,snet::T<2,89>,snet::T<3,78>,snet::T<4,68>,snet::T<5,-111>,snet::T<6,-62>,snet::T<7,-97>,snet::T<8,-1>,snet::T<9,67>,snet::T<10,-12>,snet::T<11,-7>,snet::T<12,39>,snet::T<13,-30>,snet::T<14,74>,snet::T<15,-74>,snet::T<16,-103>,snet::T<17,-3>,snet::T<18,-120>,snet::T<19,101>,snet::T<20,86>,snet::T<21,-28>,snet::T<22,-17>,snet::T<23,28>,snet::T<24,68>,snet::T<25,69>,snet::T<26,-127>,snet::T<27,51>,snet::T<28,-13>,snet::T<29,-59>,snet::T<30,57>,snet::T<31,78>,snet::T<32,-69>,snet::T<33,24>,snet::T<34,114>,snet::T<35,-101>,snet::T<36,103>,snet::T<37,-46>,snet::T<38,-120>,snet::T<39,-122>,snet::T<40,-121>,snet::T<41,39>,snet::T<42,11>,snet::T<43,-125>,snet::T<44,113>,snet::T<45,98>,snet::T<46,-30>,snet::T<47,48>,snet::T<48,-72>,snet::T<49,121>,snet::T<50,-19>,snet::T<51,58>,snet::T<52,-120>,snet::T<53,8>,snet::T<54,-71>,snet::T<55,68>,snet::T<56,-15>,snet::T<57,113>,snet::T<58,-1>,snet::T<59,14>>,lin::Bias<-2000>>>;
volatile uint8_t in_[60]; volatile int16_t out_;
int main(){ for(;;){ wave::Features<60> f; for(int i=0;i<60;i++) f.v[i]=in_[i]; out_=Net::proc<0>(f);} }
