// net_expand.cpp — snet::Net as a HAPI container (descentUnify Phase 4). Compile-only.
// A Net says what it holds (its cells) through hapi::Expand, so HAPI's walks can open it:
//   queries, Filter/Map/Partition and FindFirst look inside; rules() are not run inside it (nothing nests a Net today).
// The cells themselves stay whole: a cell is an APIOf, a leaf for queries and Filter/Map (plan D3), so what a walk
// sees inside a Net is cells, never a cell's own components. refid keeps FromTypes for exactly that reason.
#include "waveCell.h"
#include "linCell.h"
#include "refid.h"
using namespace hapi;

namespace {
  enum : int { A=1, B=2, C=3 };
  using CellA = wave::Cell<0,  Tag<A>, wave::Threshold, wave::Wave<0,0,6,0,0xff>>;
  using CellB = lin::Cell<-1,  Tag<B>, lin::Sign, lin::In<0,2>>;
  using CellC = wave::Cell<128,Tag<C>, wave::Threshold, wave::Wave<1,0,6,0,0xff>>;
  using N  = snet::Net<CellA,CellB,CellC>;
  using N0 = snet::Net<>;

  struct API {};
  struct Plain {template<typename O> using Part=O;};
  struct BadRule {
    template<typename Bf,typename Af> static constexpr bool rules() {return false;}
    template<typename O> using Part=O;
  };

  // ── what Expand says ─────────────────────────────────────────────────────────
  static_assert(IsContainer<N>::value && IsContainer<N0>::value, "a Net is a container (an empty one too)");
  static_assert(std::is_same<Expand<N>::Children,N::Cells>::value, "it holds exactly its cells, in order");
  static_assert(std::is_same<Expand<N>::Children,Chain<CellA,CellB,CellC>>::value, "...");
  static_assert( Expand<N>::queried && Expand<N>::selected && Expand<N>::searched, "queries, selection and FindFirst open it");
  static_assert(!Expand<N>::validates,                                            "rules() inside it are not run");

  // ── the cells stay whole ─────────────────────────────────────────────────────
  static_assert( Exists<SameAs<CellB>,Chain<N>>::value,    "a query sees a cell inside the Net (as a whole element)");
  static_assert(!Exists<SameAs<Tag<B>>,Chain<N>>::value,   "but not a component inside a cell: cells are leaves (D3)");
  static_assert(!Exists<SameAs<CellB>,Chain<N0>>::value,   "nothing invented from an empty Net");
  static_assert(std::is_same<Eval<Filter<SameAs<CellB>>,Chain<Plain,N>>,Chain<CellB>>::value,
    "Filter opens the Net and takes the cell whole");
  static_assert(std::is_same<Eval<Filter<FromTypes<SameAs<Tag<C>>>>,Chain<Plain,N>>,Chain<CellC>>::value,
    "the whole-object match refid relies on works through a Net too");

  // ── FindFirst opens the Net (after testing it as a whole: it has no ::Types, so a miss, then its cells) ──
  static_assert(std::is_same<typename FindFirst<FromTypes<SameAs<Tag<C>>>>::template Check<N>,CellC>::value,
    "FindFirst on the Net itself finds the cell carrying Tag<C>");
  static_assert(std::is_same<snet::CellById<B,N>,snet::CellById<B,N::Cells>>::value,
    "CellById takes the Net or its cell list: same cell");
  static_assert(std::is_same<snet::CellById<A,Chain<Plain,N>>,CellA>::value, "and a Net nested among other components");
  static_assert(!HasResult<FindFirst_<FromTypes<SameAs<Tag<7>>>,N>>::value, "an id that is not there stays a miss");

  // ── rules are not run inside a Net (D2): a failing rule in a nested Net is not seen ──
  static_assert( BuildRules<Chain<>,Chain<API,snet::Net<BadRule>>>::rules(), "rules() inside a Net are dead, like any nested container");
  static_assert(!BuildRules<Chain<>,Chain<API,BadRule>>::rules(),            "control: the same component directly does fail");
}

int main() {return 0;}
