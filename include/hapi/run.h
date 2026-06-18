/**
 * @file run.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief runtime counterpart to HAPI — chain topology at compile time, values at runtime
 */

#pragma once
#include "hapi/chain.h"
#include "hapi/meta.h"

namespace hapi {
namespace run {

  // ====================== Fold Terminals ======================
  // Identity elements for AND/OR chains — use as the base API.
  // Identity is the terminal for transform chains.

  struct True {
    template<typename T>
    constexpr bool check(const T&) const { return true; }
  };

  struct False {
    template<typename T>
    constexpr bool check(const T&) const { return false; }
  };

  // ====================== Runtime Predicates ======================
  // q is a data member — set at runtime, not a template param.
  // Each Part folds its result into O::check(v) via &&.
  // For OR chains, use the *Or variants with False as terminal.

  template<typename Q>
  struct Equal {
    template<typename O>
    struct Part : O {
      Q q{};
      bool check(const Q& v) const { return (v == q) && O::check(v); }
    };
  };

  template<typename Q>
  struct Less {
    template<typename O>
    struct Part : O {
      Q q{};
      bool check(const Q& v) const { return (v < q) && O::check(v); }
    };
  };

  template<typename Q>
  struct Greater {
    template<typename O>
    struct Part : O {
      Q q{};
      bool check(const Q& v) const { return (v > q) && O::check(v); }
    };
  };

  // OR-folded variants — use False as terminal.

  template<typename Q>
  struct EqualOr {
    template<typename O>
    struct Part : O {
      Q q{};
      bool check(const Q& v) const { return (v == q) || O::check(v); }
    };
  };

  template<typename Q>
  struct LessOr {
    template<typename O>
    struct Part : O {
      Q q{};
      bool check(const Q& v) const { return (v < q) || O::check(v); }
    };
  };

  template<typename Q>
  struct GreaterOr {
    template<typename O>
    struct Part : O {
      Q q{};
      bool check(const Q& v) const { return (v > q) || O::check(v); }
    };
  };

  // ====================== Constexpr Transform Pipeline ======================
  // Trans<F>: F is a type with constexpr operator() — zero storage (static constexpr F fn{}).
  // Composes bottom-up: outermost Trans<F> applies last.
  // Terminal must expose transform(v) — use Identity as the base API.

  struct Identity {
    template<typename T>
    constexpr T transform(T&& v) const { return std::forward<T>(v); }
  };

  template<auto fn>
  struct Trans {
    template<typename O>
    struct Part : O {
      using O::O;
      template<typename T>
      constexpr auto transform(T&& v) const {
        return fn(O::transform(std::forward<T>(v)));
      }
    };
  };

  // ====================== Runtime Expressions ======================

  template<typename P>
  struct Not {
    template<typename O>
    struct Part : P::template Part<O> {
      using Base = typename P::template Part<O>;
      template<typename T>
      bool check(const T& v) const { return !Base::check(v); }
    };
  };

  // ====================== Mutation Pipeline ======================
  // Mutate<fn>: applies fn in-place to a value, then continues chain.
  // fn is a non-type template parameter — function pointer, lambda, or functor instance.
  // Trans<fn> is functional (returns new value); Mutate<fn> is imperative (mutates ref).
  // Terminal: MutBase (do nothing).
  // Usage: APIOf<MutBase, Mutate<f1>, Mutate<f2>>::run(v) → f1(v); f2(v);

  struct MutBase {
    template<typename T>
    constexpr void run(T&) const noexcept {}
  };

  template<auto fn>
  struct Mutate {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;
      template<typename T>
      constexpr void run(T& v) const noexcept { fn(v); Base::run(v); }
    };
  };

  // ====================== Ref Pipeline ======================
  // Ref<T,F>: each component holds a runtime pointer to an external T.
  // run() applies F to *target for each Op, then chains down.
  // Targets set after construction (typically via forEach or direct assignment).
  // Terminal: RefBase (do nothing).
  // Usage: pipe.run() → F1(*t1); F2(*t2); ...

  struct RefBase {
    constexpr void run() const noexcept {}
  };

  template<typename T, typename F>
  struct Ref {
    template<typename O>
    struct Part : O {
      using Base = O;
      T* target{};
      static constexpr F fn{};
      void run() noexcept { fn(*target); Base::run(); }
    };
  };

  // ====================== CtRef Pipeline ======================
  // CtRef<I, T, F, Arr>: like Ref<T,F> but the array pointer is a NTTP —
  // baked into the type at instantiation time, zero bytes in the object.
  // Arr must have static storage duration (global/static array).
  // Equivalent to DataRef<address> in OneMenu: no runtime indirection.
  // Usage: APIOf<RefBase, CtRef<0,int,fn,g>, CtRef<1,int,fn,g>, ...>::run()
  //   → g[0] += k0; g[1] += k1; ... all as direct addressing, no pointer loads.

  template<std::size_t I, typename T, auto fn, T* Arr>
  struct CtRef {
    template<typename O>
    struct Part : O {
      using Base = O;
      void run() noexcept { fn(Arr[I]); Base::run(); }
    };
  };

  // ====================== forEach alias + visitor marker ======================
  // Checkable: inherit in the outer component struct to opt in to runEach visits.
  // runEach<Q>(node, fn): call fn on every Q-matching component — thin alias for hapi::forEach.

  struct Checkable {};

  template<typename Q, typename Node, typename Fn>
  void runEach(Node& node, Fn&& fn) {
    hapi::forEach<Q>(node, std::forward<Fn>(fn));
  }

  template<typename Q, typename Node, typename Fn>
  void runEach(const Node& node, Fn&& fn) {
    hapi::forEach<Q>(node, std::forward<Fn>(fn));
  }

} // namespace run
} // namespace hapi
