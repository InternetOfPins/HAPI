// the XOR net of sugar_check.cpp, written with sugar: size and code must equal sugar_hand_avr.cpp's (the same net by hand)
#include "waveCell.h"
#include "sugar.h"
using namespace sugar;
constexpr auto a=x<0>; constexpr auto b=x<1>;
constexpr auto orr  = cell<-1>(sign, w<2>*a,  w<2>*b);
constexpr auto nand = cell< 3>(sign, w<-2>*a, w<-2>*b);
constexpr auto xr   = cell<-3>(sign, w<2>*ref(orr), w<2>*ref(nand));
using Net = decltype(net(orr, nand, xr));
volatile uint8_t in_[2]; volatile int16_t out_;
int main(){ for(;;){ wave::Features<2> f{{in_[0],in_[1]}}; out_=Net::proc<2>(f);} }
