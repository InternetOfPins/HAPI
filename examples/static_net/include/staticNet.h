#pragma once
// staticNet.h — engine-agnostic static network composition
// Net = typelist of cell types (no instances). State is the only runtime data.
// Cells are any type with `static proc(in)` (and optionally `static update(in)`),
// pure over the state view they are given. Value types flow from each cell's proc.
#include <hapi/hapi.h>
#include <stddef.h>

#ifndef SNET_INLINE
  // avr-gcc 7.3 -Os declines to inline multiply-called proc chains (measured, see README); force it
  #define SNET_INLINE [[gnu::always_inline]]
#endif

namespace snet {
  // state view: net and current cell index as phantom types, holds only a reference
  template<typename N,typename S,size_t k> struct Ctx {
    using Net=N;
    static constexpr size_t cur=k;
    S& s;
    template<size_t j> SNET_INLINE constexpr Ctx<N,S,j> at() const {return {s};}
    template<size_t i> SNET_INLINE constexpr decltype(auto) get() const {return s.template get<i>();}
    template<size_t i,typename V> SNET_INLINE constexpr void set(V x) const {s.template set<i>(x);}
    SNET_INLINE constexpr auto data() const {return s.data();}     // the raw slot bytes, for parts that iterate over them (roll.h)
  };

  // in-place reference to cell j of the enclosing net (same pass, pure, CSE-foldable)
  template<size_t j> struct Ref {
    template<typename I> SNET_INLINE static constexpr auto get(const I& in) {
      static_assert(j<I::cur,"snet::Ref<j>: in-place reference must point to a lower net index; a cycle needs a register (Slot<i> + Store<i>)");
      return I::Net::template Cell<j>::proc(in.template at<j>());
    }
  };

  // state slot i: a true input edge, or a register written by a Store<i>
  template<size_t i> struct Slot {
    template<typename I> SNET_INLINE static constexpr auto get(const I& in) {return in.template get<i>();}
  };

  template<typename... CC> struct Net {
    using Cells=hapi::Chain<CC...>;
    template<size_t j> using Cell=typename Cells::template Drop<j>::Head;
    template<size_t j,typename S> SNET_INLINE static constexpr auto proc(S& s)
      {return Cell<j>::proc(Ctx<Net,S,j>{s});}
    template<size_t j,typename S> SNET_INLINE static constexpr void update(S& s)
      {Ctx<Net,S,j> c{s}; Cell<j>::update(c);}
  };
}

// What a Net holds, for HAPI's structural walks: its cells, in order. Queries, Filter/Map/Partition and FindFirst
// open it; rules() are not run inside it (nothing nests a Net today). The cells stay whole, a cell being an APIOf
// (a leaf for those walks), which is why refid.h matches them with FromTypes.
namespace hapi {
  template<typename... CC>
  struct Expand<snet::Net<CC...>> : Expansion<Chain<CC...>,true,true,false,true> {};
}
