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

  //HAPI-- a chain is a part of chains of parts... basically ;)

  /**
   * @brief all parts/features MUST 
   * have the singled param `O` struct named `Part` 
   * and derive from it.
   * 
   ## Feature Anatomy
   ```c++ 
   struct Gate {//part component (can have template params)
       template<typename O>
       struct Part : O {<--- Part class derives from O (mixin)
           using Base = O;
           using HasGate = std::true_type;
   
           void put(auto o) {
               if (unlocked()) Base::put(o);
           }
       };
   };
   ```
  */

  //-----------------------------------------------------------
  /// @brief Form a chain pr part, that is also a part
  /// chains can not be empty
  /// @tparam O : first part
  /// @tparam ...OO : next parts
  template<typename O,typename... OO>
  struct Chain:O::template Part<Chain<OO...>> {
    template<typename T>
    struct Part:Chain<O,OO...,T> {
      using Base=Chain<O,OO...,T>;
      template<typename C> using Join=Chain<C,O,OO...>;
      using Base::Base;
      /// @brief Declare dependencies
      /// @tparam R : a type with Require and Exclude members
      /// `Requires` will receive `O` and this `Base`
      template<typename R> using Requires= typename R::template Requires<O,Base>;
      /// @brief Declare incompatibilities
      /// @tparam R : a type with Require and Exclude members
      /// `Requires` will receive `O` and this `Base`
      template<typename R> using Excludes= typename R::template Excludes<O,Base>;
    };

    /// @brief insert `XX...` into current chain
    /// @tparam ...XX : types (parts) to be inserted
    template<typename... XX> using Ins=typename hapi::Chain<XX...,O,OO...>;
    /// @brief append  `XX...` to current chain
    /// @tparam ...XX : types (parts) to be appended
    template<typename... XX> using App=typename hapi::Chain<O,OO...,XX...>;
    template<typename C> using Join=typename C::Ins<O,OO...>;
    template<template<typename> class M> using Map=Chain<M<O>,M<OO>...>;
  };

  /// @brief last part of the chain
  /// @tparam O : the part
  template<typename O>
  struct Chain<O>:O {
    using Base=O;
    using Base::Base;

    template<typename R> using Requires=typename R::template Requires<O,Base>;
    template<typename R> using Excludes=typename R::template Excludes<O,Base>;
    template<typename... XX> using Ins=typename hapi::Chain<XX...,O>;
    template<typename... XX> using App=typename hapi::Chain<O,XX...>;
    template<typename C> using Join=typename C::Ins<O>;
    template<template<typename> class M> using Map=M<O>;
  };

  //-----------------------------------------------------------
  /// @brief parts chain default termination
  /// chains are only usable when terminated 
  /// with and API or similar class.
  /// if you do not have any, Nil can be used, 
  /// not having a fallback will be compile-time error 
  /// if no component provides the functionality (required features)
  /// a **static constexpr empty body fallback** will silently erase the code
  /// of features not included (optional features)
  struct Nil {};
  template<typename... OO> using NilPart=Chain<OO...,Nil>;

  //-----------------------------------------------------------
  /// @brief syntax sugar for API append as base of all derivations
  /// @tparam API : API class
  /// should be a single class or template with direct methods, 
  /// usually fallbacks, but not required
  template<typename API>
  struct APIOf {
    template<typename... OO>
    using Parts=hapi::Chain<OO...,API>;
  };

  //-----------------------------------------------------------
  /// @brief convenience class to form a link to the full object type
  /// CRTP can be part of API (inherited base class) and provide top level
  /// object access (type and runtime data)
  /// @tparam O : the top level object type (see CRTP online)
  template<typename O> 
  struct CRTP {
    /// @brief access the top level object type (compile time)
    using Obj=O;
    /// @brief access the object runtime instance
    /// @return this casted to the given O type
    Obj& obj() {return *(O*)this;}
    /// @brief const access the object runtime instance (constness override)
    /// @return this casted to the given const O type
    const Obj& obj() const {return *this;}
  };

};//namespace hapi
