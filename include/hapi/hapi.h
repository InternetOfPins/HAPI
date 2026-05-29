/**
 * @file hapi.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief 
 * 
*/

#pragma once

#ifdef __AVR__
  #include "platform/avr/avr_std.h"
  using Sz=unsigned int;
#else
  #include <cstddef>
  #include <type_traits>
  #include <utility>
  using Sz=size_t;
#endif

#ifdef HAPI_DEBUG
  #include <iostream>
  using std::cout;
  using std::endl;
  namespace hapi{};
#endif

#ifndef HAPI_DEBUG
  namespace hapi {
#endif

  struct Nil {};

  //query --
  template<typename Q,typename O>
  constexpr const bool query{Q::template Check<O>::value};

  // ====================== CHAIN ======================--

  template<typename...> struct Chain;

  template<> struct Chain<> {
    using Types=Chain<>;
    static constexpr const size_t size{0};
    template<typename... XX> using App=Chain<XX...>;
    template<typename... XX> using Ins=Chain<XX...>;
    template<template<typename> class M> using Map=Chain<>;
    template<typename T> struct Part:T {
      using T::T;
      using Types=Chain<T>;
    };
  };

  template<typename O,typename... OO>
  struct Chain<O,OO...> {
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

  //predicates --
  template<typename Q> struct SameAs {
    template<typename O> struct Check {
      static constexpr const bool value{std::is_same_v<O,Q>};
    };
  };

  // ====================== RULES DETECTION ======================--

  template<typename T, typename = void>
  struct HasRules : std::false_type {};

  template<typename T>
  struct HasRules<T, std::void_t<decltype(T::template rules<void,void>())>> 
    : std::true_type {};

  // ====================== BEFORE / AFTER WALK ======================--

  template<typename Before, typename Current, typename After>
  struct RuleLayer {
    template<typename O>
    struct Part : O {
      static constexpr bool rules() {
        if constexpr(HasRules<Current>::value)
          return Current::template rules<Before, After>()&&O::rules();
        else return O::rules();
      }
      // static constexpr bool rules(int){return rules();}
      // // using O::rules;
      // static constexpr std::enable_if_t<HasRules<Current>::value,bool> rules()
      //   {return T::template rules<Before, After>()&&O::rules();}
      // static constexpr std::enable_if_t<!HasRules<Current>::value,bool> rules()
      //   {return O::rules();}
    };
  };

  template<typename Before, typename After>
  struct BuildRules:
    RuleLayer<Before,typename After::Head,typename After::Tail>::template Part<
      hapi::BuildRules<typename Before::template App<typename After::Head>, typename After::Tail>
    >
  {};

  template<typename Before>
  struct BuildRules<Before,Chain<>> {
    static constexpr bool rules() {return true;}
  };

  // ====================== APIOf ======================--

  template<typename API, typename... OO>
  struct APIOf : Chain<OO...>::template Part<API> {
    using Base = typename Chain<OO...>::template Part<API>;
    using Base::Base;
    static_assert((BuildRules<Chain<>,Chain<OO...>>::rules(),true), "HAPI: validation failed");//will never fail here
  };

  template<typename API>
  struct APIOf<API> : API { 
    using Base = API; 
    using Base::Base; 
  };

  //optional, use only if your API needs it --
  template<typename O>
  struct CRTP {
    using Obj=O;
    O& obj() {return static_cast<O&>(*this);}
    const O& obj() const {return static_cast<const O&>(*this);}
    
    O* operator->() {return static_cast<O*>(this);}
    const O* operator->() const {return static_cast<const O*>(this);}
  };

#ifndef HAPI_DEBUG
  };//namespace hapi
#endif

