#pragma once

/**
 * @file hapi.h
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief API of api's
 *  provide base definitions for compositional API's
 * @version 1
 * @date 2025-04-09
 * @copyright MIT licence
 * 
 */

namespace hapi {

  //parts chain default termination
  struct Nil {};

  template<typename...> struct Parts;
  template<> struct Parts<> {template<typename P> using Part=P;};
  template<typename O> struct Parts<O>:O {};
  template<typename O,typename... OO> struct Parts<O,OO...>:O::template Part<Parts<OO...>>{
    template<typename P> using Part=hapi::Parts<O,OO...,P>;
  };

  template<typename API>
  struct APIOf {
    template<typename... OO>
    using Parts=hapi::Parts<OO...,API>;
  };

  /// @brief make a Nil terminated part
  /// @tparam ...OO members parts
  template<typename... OO> using NilPart=Parts<OO...,Nil>;

  template<typename O,typename Fallback=Nil>
  struct CRTP:Fallback {//optional
    static constexpr const bool hasCRTP=true;
    using Obj=O;
    const O& obj() const {return *this;}
    Obj& obj() {return *reinterpret_cast<Obj*>(this);}
  };
};
