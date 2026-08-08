/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief HLS security-logic smoke test: a CAN transmit-disabler gate built
 *        as one Chain<> stack -- a compile-time (mailbox,id) whitelist
 *        (stateless Allow<> layers, same shape as hls_smoke's WrapWith<>)
 *        composed under a stateful MinCycle<> replay/flood guard (one
 *        shared last-tick register + armed flag, same "each layer owns
 *        real state" idea hls_fir's Tap<> demonstrates). Checks whether
 *        HAPI's composition model reads naturally for access-control
 *        logic, not just DSP taps -- a different application domain than
 *        hls_fir/hls_smoke/OneParse's JSON grammar, same underlying
 *        Chain<>/APIOf<> machinery.
 * @date 2026-08-08
 *
 */
#include <hapi/hapi.h>
#include <cstdint>
#include <cstdio>
#include <type_traits>
using namespace hapi;

/// @brief innermost layer: ran out of whitelist entries, not permitted.
struct Deny {
  static bool allowed(uint8_t /*mailbox*/, uint16_t /*id*/) { return false; }
};

/// @brief one whitelist entry: this (mailbox,id) pair may transmit.
/// Stateless -- same "pure function per layer" shape as hls_smoke's
/// WrapWith<>, chained via logical OR down to Deny's unconditional false.
template<uint8_t Mailbox, uint16_t Id>
struct Allow {
  template<typename I>
  struct Part : I {
    using Base = I;
    using Base::Base;
    static bool allowed(uint8_t mailbox, uint16_t id) {
      return (mailbox == Mailbox && id == Id) || I::allowed(mailbox, id);
    }
  };
};

/// @brief stateful outer layer: gates the whitelist result behind a
/// minimum-interval check against the last permitted transmission.
/// `tick` is a caller-supplied counter value, not a live hardware timer --
/// see README.md's open question on whether that is a fair match to a
/// bus-timing-driven minimum-cycle check.
template<uint32_t MinInterval>
struct MinCycle {
  template<typename I>
  struct Part : I {
    using Base = I;
    using Base::Base;
    uint32_t lastTick{0};
    bool armed{false};

    bool permit(uint8_t mailbox, uint16_t id, uint32_t tick) {
      bool wlOk    = I::allowed(mailbox, id);
      bool cycleOk = !armed || (tick - lastTick >= MinInterval);
      bool ok      = wlOk && cycleOk;
      if (ok) { lastTick = tick; armed = true; }
      return ok;
    }
  };
};

// Three legit (mailbox,id) pairs, minimum 100-tick interval between any
// two permitted transmissions.
using Guard = Chain<MinCycle<100>, Allow<0, 0x100>, Allow<1, 0x101>, Allow<2, 0x102>>;
using DisablerTop = APIOf<Deny, Guard>;

/// @brief compile-time regression guards -- same spirit as hls_fir's.
static_assert(!std::is_polymorphic_v<DisablerTop>,
  "HAPI regression: Chain/APIOf collapse must stay non-polymorphic (no vtable)");
static_assert(std::is_trivially_destructible_v<DisablerTop>,
  "HAPI regression: collapsed chain should stay trivially destructible");
/// One uint32_t tick register + one bool, padded -- the whitelist layers
/// are stateless and contribute no storage. Pinning the exact size catches
/// any future layer silently gaining extra state/padding.
static_assert(sizeof(DisablerTop) == 8,
  "HAPI regression: disabler should cost exactly one tick register + one flag, no padding beyond alignment");

int main() {
  bool allPass = true;
  auto check = [&](const char* label, bool got, bool expect) {
    bool pass = (got == expect);
    allPass &= pass;
    std::printf("%-28s got=%-5s expect=%-5s %s\n", label,
                got ? "true" : "false", expect ? "true" : "false",
                pass ? "PASS" : "FAIL");
  };

  // Fresh instance per scenario group so cases don't leak MinCycle state
  // into each other -- each group below re-derives its own DisablerTop.

  {
    DisablerTop d;
    // legit transmit permit: mailbox 0, id 0x100 is whitelisted, first
    // transmission ever (not armed yet) -- cycle check is skipped.
    check("legit transmit", d.permit(0, 0x100, 0), true);
  }
  {
    DisablerTop d;
    // mailbox-3 spoof block: id 0x100 IS whitelisted, but not for
    // mailbox 3 -- Allow<> checks the (mailbox,id) pair, not id alone.
    check("mailbox-3 spoof", d.permit(3, 0x100, 0), false);
  }
  {
    DisablerTop d;
    // id-0x200 spoof block: no mailbox is whitelisted for id 0x200.
    check("id-0x200 spoof", d.permit(0, 0x200, 0), false);
  }
  {
    DisablerTop d;
    // in-cycle retransmit block: same legit pair retransmitted 50 ticks
    // later, below the 100-tick minimum interval.
    d.permit(0, 0x100, 0);
    check("in-cycle retransmit", d.permit(0, 0x100, 50), false);
  }
  {
    DisablerTop d;
    // past-cycle retransmit permit: same legit pair retransmitted 150
    // ticks later, past the 100-tick minimum interval.
    d.permit(0, 0x100, 0);
    check("past-cycle retransmit", d.permit(0, 0x100, 150), true);
  }

  std::printf("\n%s\n", allPass ? "ALL PASS" : "SOME FAILED");
  return allPass ? 0 : 1;
}
