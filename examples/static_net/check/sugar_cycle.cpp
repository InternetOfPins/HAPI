#include "waveCell.h"
#include "sugar.h"
using namespace sugar;
constexpr auto a=x<0>;
constexpr auto c0 = cell<0>(sign, w<1>*a);
constexpr auto c1 = cell<0>(sign, w<1>*ref(c0));
using Bad = decltype(net(c1, c0));     // c1 placed before the cell it reads
int main(){ wave::Features<1> f{{0}}; return Bad::proc<0>(f); }
