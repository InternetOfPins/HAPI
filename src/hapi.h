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
  };

  template<typename O,typename... OO>
  struct Chain<O,OO...> {

    using Head=O;
    using Tail=Chain<OO...>;

    static constexpr const Sz size{1+sizeof...(OO)};

    template<template<typename...> class T> using Build=T<O,OO...>;

    // template<typename Predicate,typename Comp>
    // static constexpr const bool query() {return Query<Predicate,O,Comp>::value||(Query<Predicate,OO,Comp>::value||...);}

    //here we only check the Chain remaining...
    // template<typename T>
    // static constexpr const bool Has = std::is_same_v<T, O> || (std::is_same_v<T, OO> || ...);

    template<typename... XX> using Append = Chain<O,OO...,XX...>;
    template<typename... XX> using Prepend = Chain<XX...,O,OO...>;
    template<typename X> struct Join:Chain<O,OO...,X>{};
    template<typename... XX> struct Join<Chain<XX...>>:Chain<O,OO...,XX...>{};

    template<typename T>
    struct Part:Chain<O,OO...,T> {
      using Base=Chain<O,OO...,T>;
    };
  };

  // ====================== MAIN API ======================

  template<typename API, typename... OO>
  struct APIOf : Chain<OO...>::template Part<API> {
    using Base = typename Chain<OO...>::template Part<API>;
    using Base::Base;
    // template<typename T> static constexpr const bool same=std::is_same_v<O,T>||OrAny<std::is_same_v<OO,T>...>::value;
  };

}; //namespace hapi 

template<typename Crit>
struct IsSame {
  template<typename Target,typename Comp=hapi::Chain<>>
  static constexpr const bool value{std::is_same_v<Crit,Target>};
};

template<typename Predicate>
struct Has {
  template<typename Ahead,typename Behind=hapi::Chain<>>
  static constexpr const bool value{Predicate::template value<typename Ahead::Head,typename Ahead::template Join<Behind>>};
  struct Before {
    template<typename Ahead,typename Behind=hapi::Chain<>>
    static constexpr const bool value{Predicate::template value<typename Ahead::Head,Ahead>};
  };
  struct After {
    template<typename Ahead,typename Behind=hapi::Chain<>>
    static constexpr const bool value{Predicate::template value<typename Ahead::Head,Behind>};
  };
};

//generic query
// template<template<typename...> class Predicate,typename Target,typename API,typename... OO>
// struct Query<Predicate,Target,hapi::APIOf<API,OO...>> {
//   typename Target::template Chk<hapi::Chain<OO...>>
//   // static constexpr const bool checks{(OO::check<Target,typename Chain<OO...>::Tail,Chain<Chain<OO...>::Head>>()||...);};
// };

// template<typename Predicate,typename API,typename... OO>
// struct Query<Predicate,Target,hapi::APIOf<API,OO...>> {
//   static constexpr const bool value{Predicate::template value<Target,hapi::Chain<OO...>>};
//   // static constexpr const bool checks{(OO::check<Target,typename Chain<OO...>::Tail,Chain<Chain<OO...>::Head>>()||...);};
// };