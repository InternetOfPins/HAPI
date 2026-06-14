/**
 * @file chain.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi chain — mono_block topology
 *        Hapi<T>::Part<O> : T::Part<O>  (delegates through T's collapse)
 *        Chain<OO...> inherits Hapi<Chain<OO...>> — Chain IS a component.
 *        Chain still defines its own Part<T> so Hapi's Part has a non-circular target.
*/

#pragma once

#ifdef __AVR__
  #include "platform/avr/avr_std.h"
  using SizeT=unsigned int;
#else
  #include <cstddef>
  #include <type_traits>
  #include <utility>
  using SizeT=size_t;
#endif

#ifdef HAPI_DEBUG
  #include <iostream>
  using std::cout;
  using std::endl;
  namespace hapi{};
#endif

#include "hapi/meta.h"

namespace hapi {
  /// @brief sentinel empty type
  struct Nil {};

  // ====================== COMPONENT ======================--

  /// @brief Wrap T as a component: Part<O> delegates through T::Part<O>.
  ///        Derived from Hapi<T> can delete or hide methods to create restricted views.
  template<typename T>
  struct Hapi {
    template<typename O>
    struct Part : T::template Part<O> {
      using Base = typename T::template Part<O>;
      using Base::Base;
      template<typename Q> constexpr auto find() const {return hapi::template find<Q>(*this);}
    };
  };

  // ====================== CHAIN ======================--

  template<typename... OO> struct Chain;

  // Empty chain — anchors recursion: Part<T> : T
  // Also inherits Hapi<Chain<>> so Chain<> is usable as a component.
  // Own Part<T> shadows Hapi's to avoid circularity.
  template<>
  struct Chain<> : Hapi<Chain<>> {
    template<typename T>
    struct Part : T { using T::T; };  // anchor: no more components, collapse to T
    using Types = Chain<>;
    static constexpr SizeT size{0};
    template<template<typename...> class W> using Build = W<>;
    template<typename... XX> using App = Chain<XX...>;
    template<typename... XX> using Ins = Chain<XX...>;
    template<template<typename> class M> using Map = Chain<>;
  };

  /// @brief Non-empty chain.
  ///        - Inherits Hapi<Chain<O,OO...>>: makes Chain usable as a component.
  ///        - Defines own Part<T>: standard collapse O::Part<Chain<OO...>::Part<T>>.
  ///          Own Part shadows Hapi's, so Hapi<Chain<O,OO...>>::Part<T>
  ///          delegates here without circularity.
  template<typename O, typename... OO>
  struct Chain<O, OO...> : Hapi<Chain<O, OO...>> {
    using Types = Chain<O, OO...>;
    using Head  = O;
    using Tail  = Chain<OO...>;
    static constexpr SizeT size{1 + sizeof...(OO)};
    template<template<typename...> class W> using Build = W<O, OO...>;
    template<typename... XX> using App = Chain<XX..., O, OO...>;
    template<typename... XX> using Ins = Chain<O, OO..., XX...>;
    template<template<typename> class M> using Map = Chain<M<O>, M<OO>...>;

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
    O& obj() {return static_cast<O&>(*this);}
    const O& obj() const {return static_cast<const O&>(*this);}
    O* operator->() {return static_cast<O*>(this);}
    const O* operator->() const {return static_cast<const O*>(this);}
  };

}; // namespace hapi
