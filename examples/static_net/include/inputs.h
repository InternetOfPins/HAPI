#pragma once
// inputs.h — stock input layers for nets whose inputs have no model of their own: the state is one slot, and a source names the field it reads.
//   inp::One    a          inp::Ab   a, b           inp::Bytes60   band[60] (one 60-element input vector)
#include "staticNet.h"

namespace inp {
  struct OneTag {};
  struct SlotOne { uint8_t a; };
  using One = hapi::APIOf<hapi::SlotApi, hapi::Slot<OneTag,SlotOne>>::Res;
  using OneA = snet::Field<OneTag,&SlotOne::a>;
  inline One one(const SlotOne& in) { One r; hapi::slot<OneTag>(r) = in; return r; }                 // one({a}): a braced list is evaluated left to right

  struct AbTag {};
  struct SlotAb { uint8_t a, b; };
  using Ab = hapi::APIOf<hapi::SlotApi, hapi::Slot<AbTag,SlotAb>>::Res;
  using A = snet::Field<AbTag,&SlotAb::a>;
  using B = snet::Field<AbTag,&SlotAb::b>;
  inline Ab ab(const SlotAb& in) { Ab r; hapi::slot<AbTag>(r) = in; return r; }                     // ab({a, b})

  struct BytesTag {};
  struct SlotBytes60 { uint8_t band[60]; };
  using Bytes60 = hapi::APIOf<hapi::SlotApi, hapi::Slot<BytesTag,SlotBytes60>>::Res;
  template<size_t k> using Band = snet::Elem<BytesTag,&SlotBytes60::band,k>;         // element k of the input vector
  inline uint8_t (&band(Bytes60& r))[60] { return hapi::slot<BytesTag>(r).band; }
}
