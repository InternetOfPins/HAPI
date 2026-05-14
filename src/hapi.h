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

using Sz=int;//size_t;

namespace hapi {template<typename...> struct Chain;};

//TODO: silent fail risk! check!
// template<typename Predicate,typename Target,typename... OO>
// struct Query {static constexpr const bool checks{false};};

namespace hapi {
  // ====================== CORE ======================

  template<>
  struct Chain<> {
    static constexpr const Sz size{0};
    template<template<typename...> class T> using Build=T<>;
    template<typename O> using Part=O;
    template<typename... XX> using Append = Chain<XX...>;
    template<typename... XX> using Prepend = Chain<XX...>;
    template<typename X> struct Join:Chain<X>{};
    template<typename... XX> struct Join<Chain<XX...>>:Chain<XX...>{};
    template<typename Predicate>
    static constexpr const bool require=false;
  };

  template<typename O,typename... OO>
  struct Chain<O,OO...> {

    using Head=O;
    using Tail=Chain<OO...>;

    template<typename Predicate>
    static constexpr const bool require=
      Predicate::template check<O>()
      ||(Predicate::template check<OO>()||...);

    template<typename T>
    struct Part:Chain<O,OO...,T> {
      using Base=Chain<O,OO...,T>;
    };

    static constexpr const Sz size{1+sizeof...(OO)};

    template<template<typename...> class T> using Build=T<O,OO...>;

    template<typename... XX> using Append = Chain<O,OO...,XX...>;
    template<typename... XX> using Prepend = Chain<XX...,O,OO...>;
    template<typename X> struct Join:Chain<O,OO...,X>{};
    template<typename... XX> struct Join<Chain<XX...>>:Chain<O,OO...,XX...>{};

  };

  // ====================== MAIN API ======================

  template<typename API, typename... OO>
  struct APIOf : Chain<OO...>::template Part<API> {
    using Base = typename Chain<OO...>::template Part<API>;
    using Base::Base;
  };

}; //namespace hapi 

template<typename Crit>
struct IsSame {
  template<typename... OO>
  static constexpr const bool check() {return (std::is_same_v<Crit,OO>||...);}
};

template<typename Predicate>
struct Has {
  template<typename Ahead,typename Behind=hapi::Chain<>>
  static constexpr const std::enable_if_t<Ahead::size==0,bool> check() {return true;}
  
  template<typename Ahead,typename Behind=hapi::Chain<>>
  static constexpr const std::enable_if_t<Ahead::size!=0,bool> check() 
    {return Ahead::template Join<Behind>::template require<Predicate>;}

  struct Before {
  template<typename Ahead,typename Behind=hapi::Chain<>>
  static constexpr const std::enable_if_t<Ahead::size==0,bool> check() {return true;}

  template<typename Ahead,typename Behind=hapi::Chain<>>
    static constexpr const std::enable_if_t<Ahead::size!=0,bool> check()
      {return Ahead::template require<Predicate>;}
  };

  struct After {
  template<typename Ahead,typename Behind=hapi::Chain<>>
  static constexpr const std::enable_if_t<Ahead::size==0,bool> check() {return true;}

  template<typename Ahead,typename Behind=hapi::Chain<>>
    static constexpr const std::enable_if_t<Ahead::size!=0,bool> check()
      {return Behind::template require<Predicate>;}
  };
};
