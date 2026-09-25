#pragma once
// linCell.h — plain integer linear cell (second engine for snet::Net, uses multiply by design)
#include "staticNet.h"
#include <stdint.h>

namespace lin {
  using snet::Net; using snet::Slot; using snet::Ref;

  // accumulator width is a template parameter: fan-in x max|input| x max|weight|
  // must fit, same constraint that limited the u8 wave cell on Sonar one level
  // down. int16_t is fine up to ~Banknote-size fan-in (4 x 255 x 127 = 130K);
  // Sonar's 60 inputs need int32_t (60 x 255 x 127 = 1.9M, overflows int16_t).
  template<typename Acc>
  struct APIOf {
    template<typename I> SNET_INLINE static constexpr Acc proc(const I&) {return 0;}
    template<typename I> SNET_INLINE static constexpr void update(I&) {}
  };
  template<typename Acc,Acc b> struct BiasOf {
    template<typename I> SNET_INLINE static constexpr Acc proc(const I& in) {return Acc(b+super::proc(in));}
  };
  // w * source value (whatever type the source yields), computed at Prod
  // width and only widened to Acc afterward, for the sum. Prod defaults to
  // Acc (no change from before this parameter existed) but can be narrower:
  // the PRODUCT of an int8-range weight and a u8 input fits comfortably in
  // int16_t (max 127*255=32385 < 32767) even when the ACCUMULATOR needs to be
  // wider (int32_t) to hold the fan-in-wide sum. On a target with no native
  // wide multiply (e.g. AVR, no hardware 32-bit MUL), forcing the multiply
  // itself down to Prod's width keeps it a single hardware instruction instead
  // of a libgcc __mulsi3/__umulhisi3-style call per term -- confirmed to
  // matter in practice, not just in theory: see the README's Sonar
  // table for the measured AVR cost of getting this wrong.
  template<typename Acc,typename Prod,typename Src,Acc w> struct TermOf {
    template<typename I> SNET_INLINE static constexpr Acc proc(const I& in) {
      return Acc(Acc(Prod(w)*Prod(Src::get(in)))+super::proc(in));
    }
  };
  template<typename Acc,typename Prod,size_t i,Acc w> using InOf=TermOf<Acc,Prod,Slot<i>,w>;
  template<typename Acc,typename Prod,size_t j,Acc w> using RefInOf=TermOf<Acc,Prod,Ref<j>,w>;
  template<typename Acc> struct SignOf {
    template<typename I> SNET_INLINE static constexpr bool proc(const I& in) {return super::proc(in)>0;}
  };
  template<typename Acc,Acc b,typename... OO> using CellOf=(OO : ... : BiasOf<Acc,b> : APIOf<Acc>);

  // int16_t-accumulator convenience aliases -- unchanged names/signatures from
  // before this was generalized, so every existing call site keeps compiling.
  // Prod=Acc here (int16_t product at int16_t accumulator was always what the
  // original code computed), so this is a pure no-op for existing callers.
  using acc=int16_t;
  using API=APIOf<acc>;
  template<acc b> using Bias=BiasOf<acc,b>;
  template<typename Src,acc w> using Term=TermOf<acc,acc,Src,w>;
  template<size_t i,acc w> using In=InOf<acc,acc,i,w>;
  template<size_t j,acc w> using RefIn=RefInOf<acc,acc,j,w>;
  using Sign=SignOf<acc>;
  template<acc b,typename... OO> using Cell=CellOf<acc,b,OO...>;
}
