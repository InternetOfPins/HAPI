#pragma once
// roll.h — a Part component whose static body (a pack of terms) is materialized as a table and iterated.
// Same terms as the unrolled form; only the realization differs.
#include "staticNet.h"
#include <stdint.h>
#ifdef __AVR__
  #include <avr/pgmspace.h>
  #define SNET_ROM PROGMEM
  #define SNET_ROM_I8(p) ((int8_t)pgm_read_byte(p))
  #define SNET_ROM_U8(p) ((uint8_t)pgm_read_byte(p))
#else
  #define SNET_ROM
  #define SNET_ROM_I8(p) (*(p))
  #define SNET_ROM_U8(p) (*(p))
#endif

namespace snet {
  // term record: input index and weight (also usable as an unrolled Part, see W below)
  template<size_t i,int w> struct T {
    static constexpr size_t index=i; static constexpr int weight=w;
    template<typename O> struct Part:O { using Base=O; using Base::Base;
      template<typename I> SNET_INLINE static constexpr auto proc(const I& in)
        {return decltype(Base::proc(in))(int16_t(int16_t(w)*int16_t(in.template get<i>())))+Base::proc(in);}
    };
  };

  template<size_t... ii> struct Dense {static constexpr bool value=true;};
  template<size_t k,size_t... ii> struct IsIota;
  template<size_t k> struct IsIota<k> {static constexpr bool value=true;};
  template<size_t k,size_t i,size_t... ii> struct IsIota<k,i,ii...> {static constexpr bool value=(i==k)&&IsIota<k+1,ii...>::value;};

  // rolled realization of a pack of T<i,w>; index column only when indices aren't 0..N-1
  template<typename... TT> struct Roll {template<typename O> struct Part:O {
    using Base=O; using Base::Base;
    static constexpr size_t n=sizeof...(TT);
    static_assert(((TT::weight>=-128&&TT::weight<=127)&&...),"snet::Roll: a weight does not fit int8 (the table stores bytes)");
    static_assert(((TT::index<256)&&...),"snet::Roll: an input index does not fit a byte");
    static constexpr bool dense=IsIota<0,TT::index...>::value;
    static constexpr int8_t  w[n]  SNET_ROM = {int8_t(TT::weight)...};
    static constexpr uint8_t ix[n] SNET_ROM = {uint8_t(TT::index)...};
    template<typename I> SNET_INLINE static auto proc(const I& in) {
      auto a=Base::proc(in);
      const uint8_t* x=in.data();
      for(uint8_t k=0;k<n;k++){
        uint8_t xi = dense ? x[k] : x[SNET_ROM_U8(&ix[k])];
        a+=decltype(a)(int16_t(int16_t(SNET_ROM_I8(&w[k]))*int16_t(xi)));
      }
      return a;
    }
  };};
  template<typename... TT> template<typename O> constexpr int8_t  Roll<TT...>::Part<O>::w[];
  template<typename... TT> template<typename O> constexpr uint8_t Roll<TT...>::Part<O>::ix[];
}
