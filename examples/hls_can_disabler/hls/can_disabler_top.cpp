// HLS synthesis target: CAN transmit-disabler gate, one Chain<> stack
// combining a compile-time mailbox/ID whitelist with a stateful minimum-
// retransmission-interval guard. Isolated in its own translation unit --
// see hls_fir's hls/fir4_top.cpp for the isolation rationale (one
// top-level global per file keeps the synthesized footprint honest).

#include <hapi/hapi.h>
#include <cstdint>
using namespace hapi;

// innermost layer: ran out of whitelist entries, not permitted.
struct Deny {
  static constexpr bool allowed(uint8_t /*mailbox*/, uint16_t /*id*/) noexcept { return false; }
};

// one whitelist entry: this (mailbox,id) pair is permitted to transmit.
template<uint8_t Mailbox, uint16_t Id>
struct Allow {
  template<typename I>
  struct Part : I {
    using Base = I;
    using Base::Base;
    static constexpr bool allowed(uint8_t mailbox, uint16_t id) noexcept {
      return (mailbox == Mailbox && id == Id) || I::allowed(mailbox, id);
    }
  };
};

// stateful outer layer: gates the whitelist result on a minimum interval
// (in caller-supplied ticks) since the last permitted transmission.
template<uint32_t MinInterval>
struct MinCycle {
  template<typename I>
  struct Part : I {
    using Base = I;
    using Base::Base;
    uint32_t lastTick{0};
    bool armed{false};

    bool permit(uint8_t mailbox, uint16_t id, uint32_t tick) noexcept {
      bool wlOk    = I::allowed(mailbox, id);
      bool cycleOk = !armed || (tick - lastTick >= MinInterval);
      bool ok      = wlOk && cycleOk;
      if (ok) { lastTick = tick; armed = true; }
      return ok;
    }
  };
};

using Whitelist = Chain<MinCycle<100>, Allow<0, 0x100>, Allow<1, 0x101>, Allow<2, 0x102>>;
using Top       = APIOf<Deny, Whitelist>;

Top disabler;

bool canDisablerTop(uint8_t mailbox, uint16_t id, uint32_t tick) noexcept {
  return disabler.permit(mailbox, id, tick);
}
