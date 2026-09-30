#pragma once
// registers.h — a value that survives from one pass to the next, as a layer of the state.
// Cells stay `proc(in)` and pure; the layer is the register: its step evaluates cells on `prev` (the whole previous state, so registers
// that read each other all see the old values) and stores the new slot. Fields no Reg names keep their previous value.
//   struct Slot { uint8_t a, b; };
//   using Regs = snet::Registers<Tag, snet::Reg<&Slot::a, CellNextA>, snet::Reg<&Slot::b, CellNextB>>;
//   using State = hapi::APIOf<snet::RegApi, snet::RegLayer<Tag, Slot, Regs>>::Res;     state.step(prev): the layers after this one first
#include "staticNet.h"

namespace snet {
  template<auto M, typename Cell> struct Reg {                    // M: the field (a member pointer) that receives the cell's value
    template<typename S, typename P> SNET_INLINE static void apply(S& s, const P& prev) {s.*M = Cell::proc(prev);}
  };
  template<typename Tag, typename... RR> struct Registers {
    template<typename Below, typename Prev> static auto run(const Below&, const Prev& prev) {
      auto s = hapi::slot<Tag>(prev);
      (RR::apply(s, prev), ...);
      return s;
    }
  };

  // the state's step: the layers after this one first, then this layer's registers; a layer without a Body holds its value
  struct RegRoot : hapi::SlotRoot { template<typename P> void step(const P&) {} };
  struct RegApi { using Res = RegRoot; };
  template<typename Tag, typename Body> struct StepOf {
    template<typename Below, typename R> struct Type : Below {
      template<typename P> void step(const P& prev) {
        Below::step(prev);
        R& self = static_cast<R&>(*this);
        if constexpr (std::is_same<Body, void>::value) self.me() = hapi::slot<Tag>(prev);
        else self.me() = Body::run(static_cast<const Below&>(self), prev);
      }
    };
  };
  template<typename Tag, typename S, typename Body = void> using RegLayer = hapi::Slot<Tag, S, StepOf<Tag,Body>::template Type>;
}
