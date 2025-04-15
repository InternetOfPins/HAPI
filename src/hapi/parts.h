#pragma once

/**
 * @file parts.h
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief API of api's
 *  provide base definitions for compositional API's
 * @version 1
 * @date 2025-04-09
 * @copyright MIT licence
 * 
 */

 #include "hapi/base.h"

 namespace hapi {
  /// @brief parts chain default termination
  struct Nil {};
  template<typename Out> Out& operator<<(Out& out,const Nil&) {return out;}

  /// @brief Parts
  /// @tparam ... composing parts list
  template<typename O,typename... OO>
  struct Parts:O::template Part<Parts<OO...>>{
    using Base=typename O::template Part<Parts<OO...>>;
    using Base::Base;
    template<typename P> using Part=hapi::Parts<O,OO...,P>;
  };
  template<typename O> struct Parts<O>:O {using O::O;};

  /// @brief create API with given base.
  /// @tparam API the base type
  template<typename API>
  struct APIOf {
    template<typename... OO>
    using Parts=hapi::Parts<OO...,API>;
  };

  /// @brief make a Nil terminated part
  /// @tparam ...OO members parts
  template<typename... OO> using NilPart=Parts<OO...,Nil>;

  /// @brief provide CRTP top reference
  /// @tparam O top type
  /// @tparam Fallback optional base for CRTP
  template<typename O,typename Fallback=Nil>
  struct CRTP:Fallback {//optional
    static constexpr const bool hasCRTP=true;
    using Obj=O;
    const O& obj() const {return *this;}
    Obj& obj() {return *reinterpret_cast<Obj*>(this);}
  };
 };