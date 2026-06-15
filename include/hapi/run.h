/**
 * @file run.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief runtime counterpart to HAPI — chain topology at compile time, values at runtime
 */

#pragma once
#include "hapi/chain.h"
#include "hapi/meta.h"
#ifndef __AVR__
  #include <array>
#endif

namespace hapi {
namespace run {

  // ====================== Tags ======================

  struct Checkable {};     ///< marks a Part that exposes check()
  struct Transformable {}; ///< marks a Part that exposes transform()

  // ====================== Fold Terminals ======================
  // Identity elements for AND/OR chains — use as the base API.

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

  // Checkable tags the *component* (outer struct), not its Part.
  // This avoids diamond inheritance when chains nest multiple Checkable components.
  // TagIs<Checkable> in fillTable checks the component type, not Part<O>.

  template<typename Q>
  struct Equal : Checkable {
    template<typename O>
    struct Part : O {
      Q q{};
      bool check(const Q& v) const { return (v == q) && O::check(v); }
    };
  };

  template<typename Q>
  struct Less : Checkable {
    template<typename O>
    struct Part : O {
      Q q{};
      bool check(const Q& v) const { return (v < q) && O::check(v); }
    };
  };

  template<typename Q>
  struct Greater : Checkable {
    template<typename O>
    struct Part : O {
      Q q{};
      bool check(const Q& v) const { return (v > q) && O::check(v); }
    };
  };

  // OR-folded variants — use False as terminal.

  template<typename Q>
  struct EqualOr : Checkable {
    template<typename O>
    struct Part : O {
      Q q{};
      bool check(const Q& v) const { return (v == q) || O::check(v); }
    };
  };

  template<typename Q>
  struct LessOr : Checkable {
    template<typename O>
    struct Part : O {
      Q q{};
      bool check(const Q& v) const { return (v < q) || O::check(v); }
    };
  };

  template<typename Q>
  struct GreaterOr : Checkable {
    template<typename O>
    struct Part : O {
      Q q{};
      bool check(const Q& v) const { return (v > q) || O::check(v); }
    };
  };

  // ====================== Runtime Expressions ======================

  template<typename P>
  struct Not : Checkable {
    template<typename O>
    struct Part : P::template Part<O> {
      using Base = typename P::template Part<O>;
      template<typename T>
      bool check(const T& v) const { return !Base::check(v); }
    };
  };

  // ====================== Runtime Traversal ======================
  // Compile-time: build a table of captureless function pointers (one per Q-matching node).
  // Runtime: loop over the table — branch-predictor-friendly, no template recursion overhead.

  namespace detail {

    // Chain-list traversal: CurChain is Chain<O,OO...> (type list, not collapsed Part).
    // Walk via CurChain::Head / CurChain::Tail — explicit template args, no pack deduction.
    // Found = Head::Part<Tail::Part<API>> is already instantiated from chain construction.
    // Found only materialised when Q is satisfied — no work on non-matching nodes.

    template<typename Q, typename CurChain>
    constexpr SizeT countMatches() {
      if constexpr (CurChain::size > 0) {
        return SizeT(Q::template Check<typename CurChain::Head>::value)
             + countMatches<Q, typename CurChain::Tail>();
      } else {
        return 0;
      }
    }

    template<typename Q, typename Fn, typename Node, typename API, typename CurChain, SizeT N>
    constexpr void fillTable(std::array<void(*)(Node&, Fn&), N>& table, SizeT& idx) {
      if constexpr (CurChain::size > 0) {
        if constexpr (Q::template Check<typename CurChain::Head>::value) {
          using Found = typename CurChain::Head::template Part<
            typename CurChain::Tail::template Part<API>>;
          table[idx++] = [](Node& n, Fn& fn) { fn(static_cast<Found&>(n)); };
        }
        fillTable<Q, Fn, Node, API, typename CurChain::Tail, N>(table, idx);
      }
    }

  } // namespace detail

  // Chains at or below this size use forEach (fully inlined, optimizer-visible).
  // Larger chains switch to the dispatch table + runtime loop (flat stack).
  // Override by defining HAPI_RUN_INLINE_MAX before including this header.
  #ifndef HAPI_RUN_INLINE_MAX
    static constexpr SizeT RunInlineMax = 8;
  #else
    static constexpr SizeT RunInlineMax = HAPI_RUN_INLINE_MAX;
  #endif

  // runEach<Q>(node, fn): visits every Q-matching node.
  // Strategy chosen at compile time from N: inline fold (small) or table loop (large).
  template<typename Q, typename Node, typename Fn>
  void runEach(Node& node, Fn fn) {
    using API   = typename Node::Types::Head;
    using Hapis = typename Node::Types::Tail;
    static constexpr SizeT N = detail::countMatches<Q, Hapis>();
    if constexpr (N <= RunInlineMax) {
      forEach<Q>(node, fn);
    } else {
      using FnPtr = void(*)(Node&, Fn&);
      static constexpr auto table = []() constexpr {
        std::array<FnPtr, N> t{};
        SizeT idx = 0;
        detail::fillTable<Q, Fn, Node, API, Hapis, N>(t, idx);
        return t;
      }();
      for (auto f : table) f(node, fn);
    }
  }

  template<typename Q, typename Node, typename Fn>
  void runEach(const Node& node, Fn fn) {
    using API   = typename Node::Types::Head;
    using Hapis = typename Node::Types::Tail;
    static constexpr SizeT N = detail::countMatches<Q, Hapis>();
    if constexpr (N <= RunInlineMax) {
      forEach<Q>(node, fn);
    } else {
      using FnPtr = void(*)(const Node&, Fn&);
      static constexpr auto table = []() constexpr {
        std::array<FnPtr, N> t{};
        SizeT idx = 0;
        detail::fillTable<Q, Fn, const Node, API, Hapis, N>(t, idx);
        return t;
      }();
      for (auto f : table) f(node, fn);
    }
  }

} // namespace run
} // namespace hapi
