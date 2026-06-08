/**
 * @file base.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi common content
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

  // ====================== CHAIN ======================--

  // Base primary template: empty chain definition
  template<typename... OO> 
  struct Chain {
    using Types=Chain<OO...>;
    static constexpr const SizeT size{0};
    // Assemble container W with same components
    template<template<typename...> class W> using Build=W<>;
    template<typename... XX> using App=Chain<XX...>;
    template<typename... XX> using Ins=Chain<XX...>;
    template<template<typename> class M> using Map=Chain<>;
    template<typename T> struct Part:T {using T::T;};
  };

  /// @brief a chain of types
  /// @tparam O : first types (head)
  /// @tparam OO... : remaining types (for tail)
  template<typename O,typename... OO>
  struct Chain<O,OO...> {
    /// introspection
    using Types=Chain<O,OO...>;
    /// chain Head
    using Head=O;
    /// chain tail
    using Tail=Chain<OO...>;
    /// chain size
    static constexpr const SizeT size{1+sizeof...(OO)};
    // Assemble container W with same components
    template<template<typename...> class W> using Build=W<O,OO...>;
    /// append
    template<typename... XX> using App=Chain<XX...,O,OO...>;
    /// insert
    template<typename... XX> using Ins=Chain<O,OO...,XX...>;
    /// map a transformation over the chain
    template<template<typename> class M> using Map=Chain<M<O>,M<OO>...>;

    // @brief collapse types O,OO... into a single c++ object forming an inheritance chain overt it's internal class `Part<>`
    // @tparam T : the termination object
    template<typename T>
    struct Part:          O::template Part<typename Chain<OO...>::template Part<T>> {
      using Base=typename O::template Part<typename Chain<OO...>::template Part<T>>;
      using Base::Base;
      using Types=Chain<O,OO...>;
    };
  };

  /// @brief provide circular reference to the whole chain if needed
  /// optional, use only if your API needs that any component can 
  /// call upper/derived object members.
  /// @note: this will cause your error messages to be even bigger and can lead to loops if not properly used.
  /// @tparam O the final collapsed object complete type
  template<typename O>
  struct CRTP {
    using Obj=O;
    O& obj() {return static_cast<O&>(*this);}
    const O& obj() const {return static_cast<const O&>(*this);}
    
    O* operator->() {return static_cast<O*>(this);}
    const O* operator->() const {return static_cast<const O*>(this);}
  };

};
