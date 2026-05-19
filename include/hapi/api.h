/**
 * @file hapi.h
 * @brief The Happy API - Composition API build tools.
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @version beta
 * @contributor Grok (xAI) - architecture, cleanup & modern C++ patterns
 * @copyright MIT licence
*/

#pragma once

#ifdef ARDUINO
  #include <Arduino.h>
#endif

#include "hapi/comp.h"
namespace hapi {
  
  //API specific composite assembler
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
      using Base=typename Head::template Part<typename Tail::template Part<T>>;
      using Base::Base;
      template<typename Q>
      static constexpr const bool has{Chain::template has<Q>};
    };
  };

  //empty chain
  template<> struct Chain<>:Composite<Chain> {
    template<typename T> using Part=T;
    template<typename Predicate>
    static constexpr const bool require=false;
  };

  //optional, use only if your API needs it
  template<typename O>
  struct CRTP {
    O& obj() {return static_cast<O&>(*this);}
    const O& obj() const {return static_cast<const O&>(*this);}
    
    O* operator->() {return static_cast<O*>(this);}
    const O* operator->() const {return static_cast<const O*>(this);}
  };

  template<typename API, typename... OO>
  struct APIOf : Chain<OO...>::template Part<API> {
    using Base = typename Chain<OO...>::template Part<API>;
    using Base::Base;
  };


}; //namespace hapi 
