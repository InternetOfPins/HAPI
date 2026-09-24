#pragma once
// refid.h — reference a cell by a QUERY on its components, resolved at compile time with HAPI. An id is just one criterion.
//   RefQ<Q>    the FIRST cell of the enclosing net whose component list (APIOf::Types) satisfies the HAPI predicate Q
//              (FindFirst semantics; the cell is matched whole). Usable wherever a Ref<j> is: same pass, in place, same
//              ordering rule as Ref<j> (it *is* Ref<j> once resolved). No match is a compile error that says so.
//   RefId<id>  = RefQ<SameAs<Tag<id>>>: the cell tagged with hapi::Tag<id> (a zero-cost pass-through Part).
// Nothing here says a match must be unique: several cells may legitimately match and the first one wins. A net that needs
// uniqueness states it as its own rule, e.g. static_assert(CellsMatching<Q,N>::size==1): a rule the user adds, not a property
// of the lookup.
#include "staticNet.h"

namespace snet {
  template<typename T,typename L> struct IndexOf;                       // position of T in a Chain
  template<typename T,typename... CC> struct IndexOf<T,hapi::Chain<T,CC...>> {static constexpr size_t value=0;};
  template<typename T,typename H,typename... CC> struct IndexOf<T,hapi::Chain<H,CC...>>
    {static constexpr size_t value=1+IndexOf<T,hapi::Chain<CC...>>::value;};

  // the cells of a Net, as HAPI sees them (Expand), not by reaching into Net::Cells
  template<typename N> using CellsOf=typename hapi::Expand<N>::Children;

  // the first cell of L (a Net or a Chain of cells) whose component list satisfies Q. Hard-fails with a HAPI-internal
  // message when there is none: RefQ goes through QIndex, which says it in words.
  template<typename Q,typename L>
  using CellBy=typename hapi::FindFirst<hapi::FromTypes<Q>>::template Check<L>;
  template<int id,typename L> using CellById=CellBy<hapi::SameAs<hapi::Tag<id>>,L>;

  // every cell of N satisfying Q, whole, in order (a Chain): the building block for a uniqueness rule of your own
  template<typename Q,typename N> using CellsMatching=hapi::Eval<hapi::Filter<hapi::FromTypes<Q>>,N>;

  namespace detail { template<typename> inline constexpr bool never=false; }   // dependent false: fires only when instantiated

  template<typename Q,typename N,bool found=hapi::HasResult<hapi::FindFirst_<hapi::FromTypes<Q>,N>>::value> struct QIndex {
    static_assert(detail::never<Q>,"snet::RefQ<Q>: no cell in the net matches Q (for RefId<id>: no cell carries Tag<id>)");
    static constexpr size_t value=0;
  };
  template<typename Q,typename N> struct QIndex<Q,N,true> {static constexpr size_t value=IndexOf<CellBy<Q,N>,CellsOf<N>>::value;};

  template<typename Q> struct RefQ {
    template<typename I> SNET_INLINE static constexpr auto get(const I& in) {
      return Ref<QIndex<Q,typename I::Net>::value>::get(in);
    }
  };
  template<int id> using RefId=RefQ<hapi::SameAs<hapi::Tag<id>>>;
}
