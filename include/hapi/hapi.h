/**
 * @file hapi.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief A powerful modular, zero-overhead, static composition engine for embedded systems and modern C++.
 * */

#pragma once
#include "hapi/rules.h"
#include "hapi/meta.h"
#include "hapi/reg.h"

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

    static_assert(BuildRules<Chain<>,Chain<OO...>>::rules(), "HAPI: validation failed");
  };

}; // namespace hapi