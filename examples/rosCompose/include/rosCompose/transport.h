/**
 * @file transport.h
 * @brief Topic pub/sub as a HAPI composition.
 *
 * Fan-out ("deliver one message to N typed handlers") is an additive
 * mono_block fold: each subscriber Part calls Base::deliver(m) then its
 * own on(m). No vtable, no std::function, no runtime container.
 *
 * The transport boundary — endpoints in another process, discovered at
 * runtime, reached over a network hop with no lookahead — is NOT a Chain
 * layer. It is a virtual cap (`Cap<Msg>`), the one place a single
 * indirect hop is irreducible. `PubLink<Msg>` holds a pointer to whatever
 * implements it (a loopback stub, a socket sender, rcl_publish).
 */
#pragma once
#include <hapi/hapi.h>

namespace rosCompose {

  using hapi::Chain;

  /// End of a fan-out chain — deliver() bottoms out here.
  template<typename Msg>
  struct PubCap { static void deliver(const Msg&) {} };

  /// A subscriber. Supply a Body with `on(const Msg&)`; the fold wiring
  /// is identical for every subscriber.
  template<typename Msg, typename Body>
  struct Subscriber {
    template<typename T>
    struct Part : Body::template Part<T> {
      using Base = typename Body::template Part<T>;
      using Base::Base;
      void deliver(const Msg& m) { Base::deliver(m); this->on(m); }
    };
  };

  /// Fan-out over a compile-time-fixed set of local subscribers.
  ///   using Topic = LocalFanout<Msg, SubA, SubB, SubC>;
  template<typename Msg, typename... Subs>
  using LocalFanout = typename Chain<Subs...>::template Part<PubCap<Msg>>;

  /// The causal-boundary transport seam. The one irreducible indirect hop.
  template<typename Msg>
  struct Cap {
    virtual void deliver(const Msg& m) = 0;
    virtual ~Cap() = default;
  };

  /// Chain terminal that forwards to a transport `Cap` if one is linked.
  /// Non-polymorphic in its own layout — the vtable lives behind the
  /// pointer, so a fire-and-forget publisher stays a plain Chain layer.
  ///   using Publisher = Chain<SubA, SubB>::Part<PubLink<Msg>>;
  template<typename Msg>
  struct PubLink {
    Cap<Msg>* link = nullptr;
    void deliver(const Msg& m) { if (link) link->deliver(m); }
  };

}
