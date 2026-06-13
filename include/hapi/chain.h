/**
 * @file chain.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi chain — mono_block topology
 *        Chain is a Component: inherits Component<Chain<OO...>>, Part is inherited not defined.
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

namespace hapi {
  /// @brief sentinel empty type
  struct Nil {};

  // ====================== COMPONENT ======================--

  /// @brief provides Part<O> : T, O for any type T
  /// Chain inherits this — so Part is inherited, not separately defined
  template<typename T>
  struct Component {
    template<typename O>
    struct Part : T, O {
      using T::T;
    };
  };

  // ====================== CHAIN ======================--

  template<typename... OO> struct Chain;

  // Empty chain: IS a Component — Part<O> : Chain<>, O
  template<>
  struct Chain<> : Component<Chain<>> {
    using Types = Chain<>;
    static constexpr SizeT size{0};
    template<template<typename...> class W> using Build = W<>;
    template<typename... XX> using App = Chain<XX...>;
    template<typename... XX> using Ins = Chain<XX...>;
    template<template<typename> class M> using Map = Chain<>;
  };

  /// @brief Non-empty chain: inherits head component O, tail Chain<OO...>,
  ///        and Component<Chain<O,OO...>> which provides Part<T> : Chain<O,OO...>, T.
  ///        Part is pulled in explicitly to disambiguate from Chain<OO...>'s Part.
  template<typename O, typename... OO>
  struct Chain<O, OO...> : O, Chain<OO...>, Component<Chain<O, OO...>> {
    using Component<Chain<O, OO...>>::Part;  // disambiguate: use own Part, not tail's
    using Types = Chain<O, OO...>;
    using Head  = O;
    using Tail  = Chain<OO...>;
    static constexpr SizeT size{1 + sizeof...(OO)};
    template<template<typename...> class W> using Build = W<O, OO...>;
    template<typename... XX> using App = Chain<XX..., O, OO...>;
    template<typename... XX> using Ins = Chain<O, OO..., XX...>;
    template<template<typename> class M> using Map = Chain<M<O>, M<OO>...>;
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
