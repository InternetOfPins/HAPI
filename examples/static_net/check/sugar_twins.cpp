// Cells of identical type are one cell to ref(): it resolves to the first. Intended: identical pure cells compute the same value,
// so that is deduplication. Pinned here so a change to it is a decision, not an accident.
#include "waveCell.h"
#include "sugar.h"
#include <cstdio>
using namespace sugar;
constexpr auto a=x<0>;
constexpr auto c0 = cell<0>(sign, w<1>*a);
constexpr auto c1 = cell<0>(sign, w<1>*a);                 // the same type as c0
static_assert(std::is_same_v<decltype(c0),decltype(c1)>);
constexpr auto c2 = cell<0>(sign, w<1>*ref(c1));           // "c1" is cell 0, the first cell of that type
using N = decltype(net(c0, c1, c2));
template<typename T> using U=std::remove_cv_t<T>;          // decltype of a constexpr variable is const; the net holds the plain type
static_assert(CellIndex<U<decltype(c1)>,N::Cells>::value==0, "ref(c1) resolves to the first cell of that type");
static_assert(CellIndex<U<decltype(c2)>,N::Cells>::value==2);
int main(){ int ok=1; for(int p=0;p<2;p++){ wave::Features<1> f{{(wave::u8)p}}; ok&=N::proc<2>(f)==(p>0); }
  printf("twins %s\n",ok?"OK":"FAIL"); return !ok; }
