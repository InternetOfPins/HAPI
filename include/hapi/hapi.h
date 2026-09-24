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

  /// @brief an APIOf holds its API first, then its components: exactly Types, and
  /// what BuildRules/NoCollision walk when they splice a nested APIOf. Only `validates`
  /// is on: today those two splice a nested APIOf, while Traverse-based queries,
  /// Filter/Map, and FindFirst treat it as a leaf (whole element).
  template<typename API, typename... OO>
  struct Expand<APIOf<API,OO...>> : Expansion<Chain<API,OO...>,false,false,true,false> {};

}; // namespace hapi