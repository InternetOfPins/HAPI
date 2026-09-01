/**
 * @file service.h
 * @brief Request/response as two correlated causal-boundary hops.
 *
 * A service reuses the topic transport `Cap` twice — one hop per
 * direction — correlated by a token carried alongside the payload
 * (`Wrapped<T>`, the stand-in for rmw_request_id_t). The client tracks
 * in-flight requests in a fixed-capacity table (no heap); the service is
 * stateless per request. Both endpoints are polymorphic — a bidirectional
 * endpoint is the far side of the other's pointer, so it must implement
 * `Cap` itself (unlike a one-directional `PubLink`).
 */
#pragma once
#include "rosCompose/transport.h"

namespace rosCompose {

  template<typename T>
  struct Wrapped { int token; T payload; };

  /// Fixed-capacity table of pending requests. No heap, linear scan.
  template<typename Resp, int N>
  struct PendingTable {
    using Cb = void(*)(const Resp&);
    Cb   cbs[N]  = {};
    bool used[N] = {};
    int alloc(Cb cb) {
      for (int i = 0; i < N; ++i) if (!used[i]) { used[i] = true; cbs[i] = cb; return i; }
      return -1;
    }
    void resolve(int token, const Resp& r) {
      if (token < 0 || token >= N || !used[token]) return;
      used[token] = false;
      cbs[token](r);
    }
  };

  /// Send request one way, receive one correlated response the other.
  /// N = max requests in flight.
  template<typename Req, typename Resp, int N>
  struct Client : Cap<Wrapped<Resp>> {
    Cap<Wrapped<Req>>* linkOut = nullptr;
    PendingTable<Resp, N> table;

    void call(const Req& req, typename PendingTable<Resp, N>::Cb cb) {
      int token = table.alloc(cb);
      if (linkOut) linkOut->deliver(Wrapped<Req>{token, req});
    }
    void deliver(const Wrapped<Resp>& w) override { table.resolve(w.token, w.payload); }
  };

  /// Receive request, run a handler, send the response back. Stateless.
  template<typename Req, typename Resp>
  struct Service : Cap<Wrapped<Req>> {
    Cap<Wrapped<Resp>>* linkOut = nullptr;
    Resp (*handler)(const Req&) = nullptr;

    void deliver(const Wrapped<Req>& w) override {
      if (!handler || !linkOut) return;
      linkOut->deliver(Wrapped<Resp>{w.token, handler(w.payload)});
    }
  };

}
