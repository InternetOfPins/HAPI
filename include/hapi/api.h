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

#include "hapi/comp.h"
namespace hapi {
  
  //API specific composite
  template<typename... Ts>
  struct Chain : Composite<Chain, Ts...> {

    using Base=Composite<Chain, Ts...>;
    using Head=typename Base::Head;
    using Tail=typename Base::Tail;

    template<typename Predicate>
    static constexpr const bool require=
      (Predicate::template check<Ts>()||...);

    template<typename T>
    struct Part:Head::template Part<typename Tail::template Part<T>> {
      using Base=typename Head::template Part<typename Chain<Ts...>::template Part<T>>;
    };
  };

  template<> struct Chain<>:Composite<Chain> {
    template<typename T> using Part=T;
    template<typename Predicate>
    static constexpr const bool require=false;
  };

  template<typename API, typename... OO>
  struct APIOf : Chain<OO...>::template Part<API> {
    using Base = typename Chain<OO...>::template Part<API>;
    using Base::Base;
  };

}; //namespace hapi 
