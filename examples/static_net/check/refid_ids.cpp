// refid.h: what the lookup sees, compile-only (g++/clang++/avr-g++). The negative case is refid_missing; runtime is refq_check.
#include "waveCell.h"
#include "linCell.h"
#include "refid.h"
using namespace snet;
namespace {
  enum : int { A=10, B=20, C=30, ABSENT=99 };
  using CA=wave::Cell<0,  hapi::Tag<A>, wave::Threshold, wave::Wave<0,0,6,0,0xff>>;
  using CB=lin::Cell<-1,  hapi::Tag<B>, lin::Sign, lin::In<0,2>>;
  using CC=wave::Cell<128,hapi::Tag<C>, wave::Threshold, wave::Wave<1,0,6,0,0xff>>;
  using N =Net<CA,CB,CC>;
  template<int id> using T=hapi::SameAs<hapi::Tag<id>>;
  static_assert(std::is_same<CellsOf<N>,hapi::Chain<CA,CB,CC>>::value,                     "the cells come from HAPI's Expand, in order");
  static_assert(std::is_same<CellsMatching<T<B>,N>,hapi::Chain<CB>>::value,                "the matching cell comes back whole");
  static_assert(CellsMatching<T<ABSENT>,N>::size == 0,                                     "an absent id matches nothing");
  static_assert(QIndex<T<A>,N>::value == 0 && QIndex<T<B>,N>::value == 1 && QIndex<T<C>,N>::value == 2, "id -> index");
  static_assert(std::is_same<CellById<C,N>,CC>::value && std::is_same<CellById<C,CellsOf<N>>,CC>::value, "a Net or its cell list");
  static_assert(std::is_same<RefId<A>,RefQ<T<A>>>::value,                                  "RefId<id> is RefQ<SameAs<Tag<id>>>");
  // an id is just one criterion: any HAPI predicate over a cell's components works
  static_assert(QIndex<hapi::SameAs<lin::Sign>,N>::value == 1,                             "the cell carrying lin::Sign");
  static_assert(QIndex<hapi::SameAs<wave::Threshold>,N>::value == 0,                       "first of several matches wins (CA, then CC)");
  static_assert(CellsMatching<hapi::SameAs<wave::Threshold>,N>::size == 2,                 "...and all of them are available to a user rule");
  // duplicates are legitimate: the same tag on two cells resolves to the first, and counting them is the user's rule to write
  using D=Net<CB,CA,CA>;
  static_assert(QIndex<T<A>,D>::value == 1,                                                "duplicate id: the first cell carrying it");
  static_assert(CellsMatching<T<A>,D>::size == 2,                                          "a uniqueness rule can count them");
}
int main(){return 0;}
