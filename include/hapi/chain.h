/**
 * @file chain.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi chain — mono_block topology.
 *        Chain<O,OO...>::Part<T> = O::Part<Chain<OO...>::Part<T>> collapses
 *        to a single inheritance stack, so a Chain is itself usable as one
 *        component inside another Chain.
*/

#pragma once

#include "hapi/meta.h"

namespace hapi {
  /// @brief sentinel empty type
  struct Nil {};

  // ====================== CHAIN ======================--

  template<typename... OO> struct Chain;

  /// Empty chain
  template<>
  struct Chain<> {
    template<typename T>
    using Part = T;  // anchor: no more components, collapse to T
    using Types = Chain<>;
    static constexpr SizeT size{0};
    template<template<typename...> class W> using Build = W<>;
    template<typename... XX> using App = Chain<XX...>;
    template<typename... XX> using Ins = Chain<XX...>;
    template<template<typename> class M> using Map = Chain<>;
  };

  // list of types
  template<typename O, typename... OO>
  struct Chain<O, OO...> {
    using Types = Chain<O, OO...>;
    using Head  = O;
    using Tail  = Chain<OO...>;
    static constexpr SizeT size{1 + sizeof...(OO)};
    template<template<typename...> class W> using Build = W<O, OO...>;
    template<typename... XX> using App = Chain<XX..., O, OO...>;
    template<typename... XX> using Ins = Chain<O, OO..., XX...>;
    template<template<typename> class M> using Map = Chain<M<O>, M<OO>...>;

    // A bare Chain<> used directly (no APIOf) has no validation hook of its
    // own -- name collisions between siblings (same method, different
    // signature -- one silently hides the other) go unchecked unless you
    // opt in yourself: see rules.h's NoCollision, usable standalone in a
    // static_assert at your own composition site.
    template<typename T>
    struct Part : O::template Part<typename Chain<OO...>::template Part<T>> {
      using Base = typename O::template Part<typename Chain<OO...>::template Part<T>>;
      using Base::Base;
      using Types = Chain<O, OO...>;
    };
  };

  /// @brief provide circular reference to the whole chain if needed
  template<typename O>
  struct CRTP {
    using Obj=O;
    [[nodiscard]] O& obj() {return static_cast<O&>(*this);}
    [[nodiscard]] const O& obj() const {return static_cast<const O&>(*this);}
    [[nodiscard]] O* operator->() {return static_cast<O*>(this);}
    [[nodiscard]] const O* operator->() const {return static_cast<const O*>(this);}
  };

}; // namespace hapi
