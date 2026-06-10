/**
 * @file meta.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi introspection filter and transformations
*/

#pragma once
#include "hapi/chain.h"

namespace hapi {
  /// @brief predicate to select elements by type
  /// @tparam Q : the selector type
  template<typename Q> struct SameAs {
    // match object O with predicate Q
    template<typename O> struct Check {
      static constexpr const bool value{std::is_same_v<O,Q>};
    };
  };

  // Variadic Logical Predicates ----------------------------
  template <typename P>
  struct Not {
    template <typename O>
    struct Check {
      static constexpr const bool value{!P::template Check<O>::value};
    };
  };

  template <typename... Ps>
  struct And {
    template <typename O>
    struct Check {
      static constexpr const bool value{(Ps::template Check<O>::value && ... && true)};
    };
  };

  template <typename... Ps>
  struct Or {
    template <typename O>
    struct Check {
      static constexpr const bool value{(Ps::template Check<O>::value || ... )};
    };
  };

  // Monadic Channels (Concrete Control Metaprogramming) ----
  template<typename T>
  struct Left {
    using Type = T;
    template<typename O> struct Check { 
      static constexpr bool value = std::is_same_v<O, Left<T>> || std::is_same_v<O, T>; 
    };
  };

  template<typename T>
  struct Right {
    using Type = T;
    template<typename O> struct Check { 
      static constexpr bool value = std::is_same_v<O, Right<T>> || std::is_same_v<O, T>; 
    };
  };

  // Universal Inspection Tool ------------------------------
  template<template<typename...> class Wrapper>
  struct IsInstanceOf {
    template<typename O> struct Check { 
      static constexpr bool value{false}; 
    };
    template<typename... Args> struct Check<Wrapper<Args...>> { 
      static constexpr bool value{true}; 
    };
  };

  // Global Convenience Aliases for the Toolset
  struct IsLeft  : IsInstanceOf<Left> {};
  struct IsRight : IsInstanceOf<Right> {};

  // Categorization Transformation ---------------------------
  template<typename Q>
  struct Partition {
    template<typename O>
    struct Apply {
      static constexpr bool value = Q::template Check<O>::value;
      using Expr = std::conditional_t<value, Right<O>, Left<O>>;
    };
  };

  // Pure 1:1 Map Topology Walker ---------------------------
  template<typename F, typename O>
  struct Map {
    using Expr = typename F::template Apply<O>::Expr;
  };

  template<typename F, typename... OO>
  struct Map<F, Chain<OO...>> {
    using Expr = Chain<typename Map<F, OO>::Expr...>;
  };

  // Conditional Extraction and Cleanup Engine --------------
  template<typename P, typename C, typename Enable = void> 
  struct FilterIf;

  template<typename P> 
  struct FilterIf<P, Chain<>> { 
    using Expr = Chain<>; 
  };

  // 1. Case for Monadic Wrappers (Excludes Chains using SFINAE to prevent ambiguity)
  template<typename P, template<typename...> class Wrapper, typename T, typename... Args, typename... OO>
  struct FilterIf<P, Chain<Wrapper<T, Args...>, OO...>, 
                  std::enable_if_t<!std::is_same_v<Wrapper<T, Args...>, Chain<T, Args...>>>> {
    static constexpr bool match = P::template Check<Wrapper<T, Args...>>::value;
    
    using Expr = std::conditional_t<
      match,
      typename FilterIf<P, Chain<OO...>>::Expr::template App<T>,
      typename FilterIf<P, Chain<OO...>>::Expr
    >;
  };

  // 2. Case for Recursive Sub-Chains within the hardware tree
  template<typename P, typename... OO, typename... Rest>
  struct FilterIf<P, Chain<Chain<OO...>, Rest...>> {
    using HeadFilter = typename FilterIf<P, Chain<OO...>>::Expr;
    using Expr = typename FilterIf<P, Chain<Rest...>>::Expr::template App<HeadFilter>;
  };

  /// @brief 
  /// @tparam Q 
  /// @tparam O 
  /// @tparam  
  template<typename Q, typename O, typename = void>
  constexpr bool query = Q::template Check<O>::value;

  template<typename Q, typename O>
  constexpr bool query<Q, O, std::void_t<typename O::Types>> = []() {
    // Check O itself first; if false, fallback to searching its internal elements
    return Q::template Check<O>::value || query<Q, typename O::Types>;
  }();

  template<typename Q, typename... XX>
  constexpr bool query<Q, Chain<XX...>> = (Q::template Check<XX>::value || ...);

  // Forward declaration of the worker helper
  template<typename Q, typename CurrentNode, bool MatchNext>
  struct WithIdHelper;

  template<typename Q, typename CurrentNode>
  constexpr auto& withId(CurrentNode& node) noexcept 
    {return WithIdHelper<Q, CurrentNode, hapi::query<Q, typename CurrentNode::Base>>::withId(node);}

  template<typename Q, typename CurrentNode>
  struct WithIdHelper<Q, CurrentNode, true> {
    static constexpr auto& withId(CurrentNode& node) noexcept 
      {return hapi::withId<Q>(static_cast<typename CurrentNode::Base&>(node));}
  };

  // BRANCH B: The next layer does NOT have it. This current layer is the target!
  template<typename Q, typename CurrentNode>
  struct WithIdHelper<Q, CurrentNode, false> {
    static constexpr auto& withId(CurrentNode& node) noexcept {return node;}
  };

};
