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

}; // namespace hapi