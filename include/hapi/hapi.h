/**
 * @file hapi.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief 
 * 
*/

#pragma once

#ifdef __AVR__
  #include "platform/avr/avr_std.h"
  using size_t=unsigned int;
#else
  #include <cstddef>
  #include <type_traits>
  #include <utility>
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

  // ====================== CHAIN ======================--

  template<typename... Ts>
  struct Chain {
    static constexpr Sz size = sizeof...(Ts);

    template<typename T>
    static constexpr bool has = (std::is_same_v<T, Ts> || ...);

    template<template<typename...> class Template>
    using Build = Template<Ts...>;

    template<typename T> using App = Chain<Ts..., T>;
    template<typename T> using Ins = Chain<T, Ts...>;
    template<typename... Us> using Concat = Chain<Ts..., Us...>;
  };

  // ====================== RULES DETECTION ======================--

  template<typename T, typename = void>
  struct HasRules : std::false_type {};

  template<typename T>
  struct HasRules<T, std::void_t<decltype(T::template rules<void,void>())>> 
    : std::true_type {};

  // ====================== BEFORE / AFTER WALK ======================--

  template<typename... Before, typename Current, typename... After>
  struct RuleLayer {
    template<typename O>
    struct Part : O {
      using Base = O;
    };

    template<typename B, typename A>
    static constexpr bool rules() {
      return Current::template rules<Chain<Before...>, Chain<After...>>();
    }
  };

  template<typename... Before, typename... Ts>
  struct BuildRules;

  template<typename... Before, typename Current, typename... Rest>
  struct BuildRules<Chain<Before...>, Current, Rest...> {
    using type = typename RuleLayer<Before..., Current, Rest...>::template Part<
                  typename BuildRules<Chain<Before..., Current>, Rest...>::type
                >;
  };

  template<typename... Before>
  struct BuildRules<Chain<Before...>> {
    using type = void;
  };

  // ====================== APIOf ======================--

  template<typename API, typename... OO> struct APIOf;

  template<typename API, typename... OO>
  struct APIOf<API, OO...> 
    : Chain<OO...>::template Build<APIOf<API, OO...>> 
  {
      using Base = typename Chain<OO...>::template Build<APIOf<API, OO...>>;
      using Base::Base;

      // Rules system with Before/After
      template<typename Comp>
      struct CheckRules {
        static constexpr bool value = AndAll<
          std::conditional_t<HasRules<OO>::value,
                            RuleLayer<Chain<>, OO, Comp>::rules<Chain<>, Comp>,
                            true>...
        >::value;
      };

      static_assert(CheckRules<Base>::value, "HAPI: validation failed");
  };

  template<typename API>
  struct APIOf<API> : API { 
      using Base = API; 
      using Base::Base; 
  };

#ifndef HAPI_DEBUG
  };//namespace hapi
#endif

