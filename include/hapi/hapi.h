/**
 * @file hapi.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief A powerful modular, zero-overhead, static composition engine for embedded systems and modern C++.
 * */

#pragma once
#include "hapi/rules.h"
#include "hapi/meta.h"

namespace hapi {
  // ====================== APIOf ======================--

  /// @brief closes chain composition with a fallback API, collapsing the chain into a
  /// single C++ class inheritance that ultimately derives from the given API.
  template<typename API, typename... OO>
  struct APIOf : Chain<OO...>::template Part<API> {
    using Base = typename Chain<OO...>::template Part<API>;
    using Base::Base;
    using Types=Chain<API,OO...>;

    /// @brief expose Part<T> for runtime find<Q> operations
    template<typename T>
    using Part = typename Chain<OO...>::template Part<T>;

    // validated over Types (API included), not just OO... — API is a real
    // component in the resulting inheritance chain and must be visible to
    // ordering/uniqueness rules like any other element.
    static_assert(BuildRules<Chain<>,Chain<API,OO...>>::rules(), "HAPI: validation failed");
  };

  /// @brief a nested APIOf used as one component is spliced into the walk in
  /// place via its own Types, exactly like rules.h's nested-bare-Chain splice —
  /// otherwise a closed composition placed as the API/fallback of an outer
  /// APIOf hides its contents from the outer chain's rules() fold, letting the
  /// same component type appear twice (once inside the nested APIOf, once in
  /// the outer OO...) without tripping any uniqueness/ordering static_assert.
  /// Kept as its own specialization here rather than folded into rules.h's
  /// splice via a generic "anything exposing ::Types" trait: that generic
  /// form was tried and reverted (see HAPI commit 7c5e779) because ::Types
  /// is a load-bearing convention on many unrelated types across the repos
  /// (ItemDef, TypedDef, StaticBody, ...), and auto-splicing all of them
  /// broke OneMenu partitionBody.h's whole-object Filter<FromTypes<...>>
  /// matching on StaticBody. Any future consolidation of these two
  /// specializations must stay keyed on the literal Chain<PP...> /
  /// APIOf<API2,OO2...> patterns, not on ::Types presence.
  template<typename Before, typename API2, typename... OO2, typename... Rest>
  struct BuildRules<Before, Chain<APIOf<API2,OO2...>, Rest...>>
    : BuildRules<Before, Chain<API2, OO2..., Rest...>> {};

  /// @brief same nested-APIOf splice as BuildRules's own above, mirrored
  /// for NoCollision (rules.h) for the same reason: a closed APIOf placed
  /// as one component must stay visible to collision checks on the outer
  /// chain, not hidden behind its own closed boundary.
  template<typename Detector, typename API2, typename... OO2, typename... Rest>
  struct NoCollision_<Detector, Chain<APIOf<API2,OO2...>, Rest...>>
    : NoCollision_<Detector, Chain<API2, OO2..., Rest...>> {};

}; // namespace hapi