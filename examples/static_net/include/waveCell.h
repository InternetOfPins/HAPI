#pragma once
// waveCell.h — conditional-free, multiply-free phase cell components (HAPI Part<O> style)
// one engine for snet::Net; the u8 cast of any source lives here, not in the composition layer
#include "staticNet.h"
#include <stdint.h>

namespace wave {
  using u8=uint8_t; // not µ: UTF-8 identifiers need GCC>=10, avr-gcc 7.3 rejects them
  using snet::Net; using snet::Ref;

  struct API {
    template<typename I> SNET_INLINE static constexpr u8 proc(const I&) {return 0;}
  };

  // global phase offset
  template<u8 k>
  struct Bias {template<typename O> struct Part:O {
    using Base=O; using Base::Base;
    template<typename I> SNET_INLINE static constexpr u8 proc(const I& in) {return u8(k+Base::proc(in));}
  };};

  // graded source: optional NOT, shift (s<0 right, s>0 left), phase add, bit-mask readout
  template<typename Src,bool n,int s,u8 p,u8 m>
  struct WaveOf {template<typename O> struct Part:O {
    using Base=O; using Base::Base;
    static constexpr u8 inv(u8 x) {return n?u8(~x):x;}
    // avr-gcc 7.3 -Os (measured): a right shift is done in 8 bits only when it reads a variable and nothing narrows it before
    // the mask; read as a call inside the expression, or cast to u8 first, it costs a register copy (.RnD/openDerivation/godbolt/).
    // A left shift is the other way round: without the early u8 it widens to 16 bits (mixed_avr_size: +30 B)
    template<typename I> SNET_INLINE static constexpr u8 proc(const I& in) {
      const u8 x=inv(u8(Src::get(in)));
      if constexpr (s>=0) return u8(u8(u8(u8(x<<s)+p)&m)+Base::proc(in));
      else return u8((((x>>(-s))+p)&m)+Base::proc(in));
    }
  };};
  template<typename Src,bool n,int s,u8 p,u8 m> using Wave=WaveOf<Src,n,s,p,m>;               // Src: a Field or Elem of the state
  template<size_t j,bool n,int s,u8 p,u8 m> using RefWave=WaveOf<Ref<j>,n,s,p,m>;

  // readout of bit 7 (phase < half)
  struct Threshold {template<typename O> struct Part:O {
    using Base=O; using Base::Base;
    template<typename I> SNET_INLINE static constexpr bool proc(const I& in) {return Base::proc(in)<128;}
  };};

  template<u8 k,typename... OO> using Cell=hapi::APIOf<API,OO...,Bias<k>>;

}
