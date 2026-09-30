#pragma once
// staticNet.h — engine-agnostic static network composition
// Net = typelist of cell types (no instances). The runtime data is a state of tag-addressed slots (hapi/slots.h): cells read it through
// sources (Field, Elem below) or read another cell (Ref). Cells are any type with `static proc(in)`, pure over the state view they are given.
// A value that must survive from one pass to the next is a typed layer that evaluates cells on `prev` (see registers.h), not a write into
// the state being read.
#include <hapi/hapi.h>
#include <hapi/hapi.h>
#include <stddef.h>

#ifndef SNET_INLINE
  // avr-gcc 7.3 -Os declines to inline multiply-called proc chains (measured, see README); force it
  #define SNET_INLINE [[gnu::always_inline]]
#endif

namespace snet {
  // state view: net and current cell index as phantom types, holds only a reference to the state
  template<typename N,typename S,size_t k> struct Ctx {
    using Net=N;
    static constexpr size_t cur=k;
    S& s;
    template<size_t j> SNET_INLINE constexpr Ctx<N,S,j> at() const {return {s};}
  };

  // the state behind a view: through Net<...>::proc<j> a source gets a Ctx, evaluated on a cell directly it gets the state itself
  template<typename N,typename S,size_t k> SNET_INLINE constexpr const S& stateOf(const Ctx<N,S,k>& c) {return c.s;}
  template<typename S> SNET_INLINE constexpr const S& stateOf(const S& s) {return s;}

  // in-place reference to cell j of the enclosing net (same pass, pure, CSE-foldable)
  template<size_t j> struct Ref {
    template<typename I> SNET_INLINE static constexpr auto get(const I& in) {
      static_assert(j<I::cur,"snet::Ref<j>: in-place reference must point to a lower net index; a cycle needs a register (a typed layer that evaluates the cell on prev)");
      return I::Net::template Cell<j>::proc(in.template at<j>());
    }
  };

  // a value that lives in the state: the field M (a member pointer) of the slot of layer Tag
  template<typename Tag,auto M> struct Field {
    template<typename I> SNET_INLINE static constexpr auto get(const I& in) {return hapi::slot<Tag>(stateOf(in)).*M;}
  };
  // element k of an array field
  template<typename Tag,auto M,size_t k> struct Elem {
    static constexpr size_t index=k;
    template<typename I> SNET_INLINE static constexpr auto get(const I& in) {return (hapi::slot<Tag>(stateOf(in)).*M)[k];}
  };

  template<typename... CC> struct Net {
    using Cells=hapi::Chain<CC...>;
    template<size_t j> using Cell=typename Cells::template Drop<j>::Head;
    template<size_t j,typename S> SNET_INLINE static constexpr auto proc(S& s)
      {return Cell<j>::proc(Ctx<Net,S,j>{s});}
  };
}

// What a Net holds, for HAPI's structural walks: its cells, in order. Queries, Filter/Map/Partition and FindFirst
// open it; rules() are not run inside it (nothing nests a Net today). The cells stay whole, a cell being an APIOf
// (a leaf for those walks), which is why refid.h matches them with FromTypes.
namespace hapi {
  template<typename... CC>
  struct Expand<snet::Net<CC...>> : Expansion<Chain<CC...>,true,true,false,true> {};
}
