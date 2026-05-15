/**
 * @file hapi.h
 * @brief The Happy API - Composition API build tools.
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @version beta
 * @date 2025-04-09
 * @copyright MIT licence
*/

#pragma once

#ifdef ARDUINO
  #include <Arduino.h>
#endif

#include <oneList.h>

namespace hapi {

  template<typename...> struct Chain;

  template<>
  struct Chain<> {
    static constexpr const Sz size{0};
    template<typename O> using Part=O;
    template<typename... XX> using App = Chain<XX...>;
    template<typename... XX> using Ins = Chain<XX...>;
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
    struct Part:O::template Part<typename Chain<OO...>::template Part<T>> {
      using Base=typename O::template Part<typename Chain<OO...>::template Part<T>>;
    };

    static constexpr const Sz size{1+sizeof...(OO)};

    template<typename... XX> using App = Chain<O,OO...,XX...>;
    template<typename... XX> using Ins = Chain<XX...,O,OO...>;
    template<typename X> struct Join:Chain<O,OO...,X>{};
    template<typename... XX> struct Join<Chain<XX...>>:Chain<O,OO...,XX...>{};

    template<template<typename> class M> using Map=Chain<M<O>,M<OO>...>;
};

  template<typename API, typename... OO>
  struct APIOf : Chain<OO...>::template Part<API> {
    using Base = typename Chain<OO...>::template Part<API>;
    using Base::Base;
  };

  template<typename O>
  struct CRTP {//optional
    static constexpr const bool hasCRTP=true;
    using Obj=O;
    const O& obj() const {return *this;}
    Obj& obj() {return *reinterpret_cast<Obj*>(this);}
  };

}; //namespace hapi 
