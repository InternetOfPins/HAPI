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

  template<typename F>
  struct Trans {
    template<typename O>
    struct Part : O {
      using O::O;
      static constexpr F fn{};
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


} // namespace run
} // namespace hapi
