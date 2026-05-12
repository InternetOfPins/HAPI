/**
 * @file hapi.h
 * @brief The Happy API - Composition API build tools.
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @version 2
 * @date 2025-04-09 v1
 * @date 2026-04-14 v2
 * @copyright MIT licence
*/

#pragma once
#include "rules.h"

namespace hapi {
  // ====================== CORE ======================

  template<typename O, typename... OO> struct Chain;
  template<typename O> struct Chain<O> {
    template<typename T> struct Part : O::template Part<T> {
      using Base = typename O::template Part<T>;
      using Base::Base;
    };
  };

  template<typename O, typename... OO> struct Chain {
    template<typename T> struct Part : O::template Part<typename Chain<OO...>::template Part<T>> {
      using Base = typename O::template Part<typename Chain<OO...>::template Part<T>>;
      using Base::Base;
    };
  };

  // ====================== MAIN API ======================

  template<typename API, typename... OO> struct APIOf;

  template<typename API, typename O, typename... OO>
  struct APIOf<API,O,OO...> : Chain<O,OO...>::template Part<API> {
    using Base = typename Chain<O,OO...>::template Part<API>;
    using Base::Base;
    // template<typename T> static constexpr const bool same=std::is_same_v<O,T>||OrAny<std::is_same_v<OO,T>...>::value;
  };

  template<typename API>
  struct APIOf<API> : API { using Base=API; using Base::Base; };

}; //namespace hapi 

// Has specialization for APIOf<...>
template<typename API, typename... Fs, typename Tag>
struct Same<hapi::APIOf<API, Fs...>,Tag> 
  {static constexpr bool value = hapi::TypeList<Fs...>::template Has<Tag>;};

// template<template<typename> class Chk,typename API,typename... OO>
// struct Has<Chk,hapi::APIOf<API, OO...>> {
//   using Comp=hapi::TypeList<OO...>;
//   static constexpr bool value = hapi::OrAny<Chk<Comp>>::value;
// };

