/**
 * @file chain.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief 
 * 
*/

#pragma once

template<typename...> struct Chain;

template<> struct Chain<>:RulesAPI {
  using Types=Chain<>;
  static constexpr const size_t size{0};
  template<typename... XX> using App=Chain<XX...>;
  template<typename... XX> using Ins=Chain<XX...>;
  template<template<typename> class M> using Map=Chain<>;
  template<typename T> struct Part:T {using Types=Chain<T>;};
};

template<typename O,typename... OO>
struct Chain<O,OO...>:RulesAPI {
  using Types=Chain<O,OO...>;
  using Head=O;
  using Tail=Chain<OO...>;
  static constexpr const size_t size{1+sizeof...(OO)};
  template<typename... XX> using App=Chain<XX...,O,OO...>;
  template<typename... XX> using Ins=Chain<O,OO...,XX...>;
  template<template<typename> class M> using Map=Chain<M<O>,M<OO>...>;

  template<typename T>
  struct Part:          O::template Part<typename Chain<OO...>::template Part<T>> {
    using Base=typename O::template Part<typename Chain<OO...>::template Part<T>>;
    using Base::Base;
    using Types=Chain<O,OO...>;
  };
};

//rules Chain query specialization --
template<typename Q,typename... OO>
constexpr const bool query<Q,Chain<OO...>>{(query<Q,OO>||...)};

//APIOf ---------------
template<typename API,typename... OO>
struct APIOf:Chain<OO...>::template Part<API>{
  using Base=typename Chain<OO...>::template Part<API>;
  using Base::Base;
  using Types=typename Base::Types;
};

//rules APIOf query specialization --
template<typename Q,typename A,typename... OO>
constexpr const bool query<Q,APIOf<A,OO...>>{(query<Q,OO>||...)};


