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

  /// @brief Close chain composition with a fallback API, 
  /// collapsing the chain into a single c++ class inheritance
  /// that ultimately derive from the given API
  /// @tparam API : fall-back API
  /// @tparam OO... : the chain components 
  template<typename API, typename... OO>
  struct APIOf : Chain<OO...>::template Part<API> {
    using Base = typename Chain<OO...>::template Part<API>;
    using Base::Base;
    using Types=Chain<API,OO...>;
    static_assert(BuildRules<Chain<>,Chain<OO...>>::rules(), "HAPI: validation failed");
  };

  // Map specialization to traverse directly through an APIOf boundary
  template<typename F, typename API, typename... OO>
  struct Map<F, APIOf<API, OO...>> {
    using Expr = APIOf<API, typename Map<F, OO>::Expr...>;
  };

  // ====================== IdxTag<I> ======================--

  /// @brief Positional index tag — zero overhead (EBO), marks component I in a chain.
  /// Place IdxTag<I> before the component it indexes:
  ///   APIOf<API, IdxTag<0>, T0, IdxTag<1>, T1, ...>
  /// Then idx<I>(node) returns a direct reference to Ti.
  template<std::size_t I>
  struct IdxTag {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;
    };
  };

  /// @brief Free function: indexed access into an IdxTag<>-tagged chain.
  /// Returns the value reference for the component tagged IdxTag<I>.
  template<std::size_t I, typename C>
  inline decltype(auto) idx(C& c) { return find<TagIs<IdxTag<I>>>(c); }

  template<std::size_t I, typename C>
  inline decltype(auto) idx(const C& c) { return find<TagIs<IdxTag<I>>>(c); }

}; // namespace hapi