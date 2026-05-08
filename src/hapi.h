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

namespace hapi {

  //HAPI--
  template<typename O,typename... OO>
  struct Chain:O::template Part<Chain<OO...>> {
    template<typename T>
    struct Part:Chain<O,OO...,T> {
      using Base=Chain<O,OO...,T>;
      using Base::Base;
      template<typename R> using Requires= typename R::template Requires<O,Base>;
      template<typename R> using Excludes= typename R::template Excludes<O,Base>;
    };
    template<typename... XX> using Ins=typename hapi::Chain<XX...,O,OO...>;
    template<typename... XX> using App=typename hapi::Chain<O,OO...,XX...>;
    template<template<typename> class M> using Map=Chain<M<O>,M<OO>...>;
  };

  template<typename O>
  struct Chain<O>:O {
    using Base=O;
    using Base::Base;
    template<typename R> using Requires=typename R::template Requires<O,Base>;
    template<typename R> using Excludes=typename R::template Excludes<O,Base>;
    template<typename... XX> using Ins=typename hapi::Chain<XX...,O>;
    template<typename... XX> using App=typename hapi::Chain<O,XX...>;
    template<template<typename> class M> using Map=M<O>;
  };

  template<typename API> struct APIOf {template<typename... OO> using Parts=hapi::Chain<OO...,API>;};

  template<typename O> 
  struct CRTP {
    using Obj=O;
    Obj& obj() {return *(O*)this;}
    const Obj& obj() const {return *this;}
  };

  struct Nil {};
  template<typename... OO> using NilPart=Chain<OO...,Nil>;

};//namespace hapi
