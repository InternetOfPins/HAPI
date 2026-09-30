// EXPECT-ERROR: sugar::ref: no cell of that type in the net
// xr reads nand, but nand is not an argument of net(...): one clear message instead of an incomplete snet::IndexOf<...>.
#include "waveCell.h"
#include "inputs.h"
#include "sugar.h"
using namespace sugar;
constexpr auto a=x<inp::OneA>;
constexpr auto orr  = cell<-1>(sign, w<2>*a);
constexpr auto nand = cell< 3>(sign, w<-2>*a);
constexpr auto xr   = cell<-3>(sign, w<2>*ref(orr), w<2>*ref(nand));
using Bad = decltype(net(orr, xr));
int main(){ auto f=inp::one({0}); return Bad::proc<1>(f); }
