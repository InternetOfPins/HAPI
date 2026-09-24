// sugar::cell picks the rolled form (snet::Roll) or the unrolled one (lin::Cell) from the term count, and the choice is invisible:
// same result either way. Pinned: the types picked, what keeps a cell unrolled, and equality with the hand-written unrolled cell.
#include "waveCell.h"
#include "sugar.h"
#include <cstdio>
#include <cstdlib>
#include <type_traits>
using namespace sugar;
template<typename T> using U=std::remove_cv_t<T>;                 // decltype of a constexpr variable is const
static_assert(rollAt==6, "the measured threshold (see the README, Realizations); this test assumes it");

// 1. the type picked
constexpr auto a=x<0>; constexpr auto b=x<1>; constexpr auto c=x<2>; constexpr auto d=x<3>; constexpr auto e=x<4>; constexpr auto f5=x<5>;
constexpr auto five = cell<7>(sign, w<2>*a, w<-3>*b, w<4>*c, w<-5>*d, w<6>*e);
constexpr auto six  = cell<7>(sign, w<2>*a, w<-3>*b, w<4>*c, w<-5>*d, w<6>*e, w<-7>*f5);
static_assert(std::is_same_v<U<decltype(five)>, lin::Cell<7,lin::Sign,lin::In<0,2>,lin::In<1,-3>,lin::In<2,4>,lin::In<3,-5>,lin::In<4,6>>>, "5 terms: unrolled");
static_assert(std::is_same_v<U<decltype(six)>, hapi::APIOf<lin::API,lin::Sign,
  snet::Roll<snet::T<0,2>,snet::T<1,-3>,snet::T<2,4>,snet::T<3,-5>,snet::T<4,6>,snet::T<5,-7>>,lin::Bias<7>>>, "6 terms: rolled");
// what keeps a cell unrolled however many terms it has (six or more here): a weight that does not fit int8 (Roll would truncate it),
// a term that reads another cell
constexpr auto wide = cell<0>(sign, w<200>*a, w<1>*b, w<1>*c, w<1>*d, w<1>*e, w<1>*f5);
static_assert(std::is_same_v<U<decltype(wide)>, lin::Cell<0,lin::Sign,lin::In<0,200>,lin::In<1,1>,lin::In<2,1>,lin::In<3,1>,lin::In<4,1>,lin::In<5,1>>>, "weight 200: unrolled");
constexpr auto first = cell<0>(sign, w<1>*a);
constexpr auto withRef = cell<0>(sign, w<1>*ref(first), w<1>*b, w<1>*c, w<1>*d, w<1>*e, w<1>*f5);
static_assert(std::is_same_v<U<decltype(withRef)>, lin::Cell<0,lin::Sign,lin::Term<RefT<U<decltype(first)>>,1>,lin::In<1,1>,lin::In<2,1>,lin::In<3,1>,lin::In<4,1>,lin::In<5,1>>>, "a ref term: unrolled");

// 2. the same result: 8 terms (dense, then gapped and out of order) against the hand-written unrolled cell, on random inputs
constexpr auto dense8 = cell<-300>(sign, w<2>*x<0>, w<-3>*x<1>, w<127>*x<2>, w<-128>*x<3>, w<5>*x<4>, w<-6>*x<5>, w<7>*x<6>, w<-8>*x<7>);
constexpr auto gap8   = cell<-300>(sign, w<2>*x<9>, w<-3>*x<1>, w<127>*x<30>, w<-128>*x<1>, w<5>*x<59>, w<-6>*x<0>, w<7>*x<9>, w<-8>*x<7>);
static_assert(!std::is_same_v<U<decltype(dense8)>, lin::Cell<-300,lin::Sign,lin::In<0,2>,lin::In<1,-3>,lin::In<2,127>,lin::In<3,-128>,lin::In<4,5>,lin::In<5,-6>,lin::In<6,7>,lin::In<7,-8>>>, "8 terms: rolled");
using SD = decltype(net(dense8));
using SG = decltype(net(gap8));
using HD = snet::Net<lin::Cell<-300,lin::Sign,lin::In<0,2>,lin::In<1,-3>,lin::In<2,127>,lin::In<3,-128>,lin::In<4,5>,lin::In<5,-6>,lin::In<6,7>,lin::In<7,-8>>>;
using HG = snet::Net<lin::Cell<-300,lin::Sign,lin::In<9,2>,lin::In<1,-3>,lin::In<30,127>,lin::In<1,-128>,lin::In<59,5>,lin::In<0,-6>,lin::In<9,7>,lin::In<7,-8>>>;
int main(){ int okD=0,okG=0,n=20000; srand(5);
  for(int t=0;t<n;t++){ wave::Features<60> f; for(int i=0;i<60;i++) f.v[i]=rand()&255;
    okD+=SD::proc<0>(f)==HD::proc<0>(f); okG+=SG::proc<0>(f)==HG::proc<0>(f); }
  printf("sugar rolled==hand unrolled: dense %d/%d, gapped %d/%d\n",okD,n,okG,n); return okD!=n||okG!=n; }
