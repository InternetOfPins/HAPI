#pragma once
// waveCell.h — conditional-free, multiply-free phase cell components (HAPI Part<O> style)
// one engine for snet::Net; the u8 cast of any source lives here, not in the composition layer
#include "staticNet.h"
#include <stdint.h>

namespace wave {
  using u8=uint8_t; // not µ: UTF-8 identifiers need GCC>=10, avr-gcc 7.3 rejects them
  using snet::Net; using snet::Slot; using snet::Ref;

  struct API {
    template<typename I> SNET_INLINE static constexpr u8 proc(const I&) {return 0;}
    template<typename I> SNET_INLINE static constexpr void update(I&) {}
  };

  // global phase offset
  template<u8 k>
  struct Bias {
    template<typename I> SNET_INLINE static constexpr u8 proc(const I& in) {return u8(k+super::proc(in));}
  };

  // graded source: optional NOT, shift (s<0 right, s>0 left), phase add, bit-mask readout
  template<typename Src,bool n,int s,u8 p,u8 m>
  struct WaveOf {
    static constexpr u8 inv(u8 x) {return n?u8(~x):x;}
    static constexpr u8 shf(u8 x) {return s>=0?u8(x<<s):u8(x>>(-s));}
    template<typename I> SNET_INLINE static constexpr u8 proc(const I& in)
      {return u8(u8(u8(shf(inv(u8(Src::get(in))))+p)&m)+super::proc(in));}
  };
  template<size_t i,bool n,int s,u8 p,u8 m> using Wave=WaveOf<Slot<i>,n,s,p,m>;
  template<size_t j,bool n,int s,u8 p,u8 m> using RefWave=WaveOf<Ref<j>,n,s,p,m>;

  // readout of bit 7 (phase < half)
  struct Threshold {
    template<typename I> SNET_INLINE static constexpr bool proc(const I& in) {return super::proc(in)<128;}
  };

  // writes the running proc() value at this point into state slot i, then chains to the next Store
  template<size_t i>
  struct Store {
    template<typename I> SNET_INLINE static constexpr void update(I& in) {
      in.template set<i>(super::proc(in));
      super::update(in);
    }
  };

  template<u8 k,typename... OO> using Cell=hapi::APIOf<API,OO...,Bias<k>>;

  // input state passed by value/reference, no global reads (CSE-eligible)
  template<size_t N> struct Features {
    u8 v[N];
    template<size_t i> constexpr u8 get() const {return v[i];}
    template<size_t i> constexpr void set(u8 x) {v[i]=x;}
    constexpr const u8* data() const {return v;}
  };
}
