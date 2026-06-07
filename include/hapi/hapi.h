/**
 * @file hapi.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief A powerful modular, zero-overhead, static composition engine for embedded systems and modern C++.
 * 
*/

#pragma once

#ifdef __AVR__
  #include "platform/avr/avr_std.h"
  using SizeT=unsigned int;
#else
  #include <cstddef>
  #include <type_traits>
  #include <utility>
  using SizeT=size_t;
#endif

#ifdef HAPI_DEBUG
  #include <iostream>
  using std::cout;
  using std::endl;
  namespace hapi{};
#endif

namespace hapi {

  /// @brief sentinel empty type
  struct Nil {};

  /// @brief apply a predicate over an object
  /// @tparam Q : predicate
  /// @tparam O : target object type
  template<typename Q,typename O>
  constexpr const bool query{Q::template Check<O>::value};

  // ====================== CHAIN ======================--

  template<typename...> struct Chain;

  // empty chain
  template<> struct Chain<> {
    static constexpr const SizeT size{0};
    template<typename... XX> using App=Chain<XX...>;
    template<typename... XX> using Ins=Chain<XX...>;
    template<template<typename> class M> using Map=Chain<>;
    template<typename T> struct Part:T {using T::T;};
  };

  /// @brief a chain of types
  /// @tparam O : first types (head)
  /// @tparam OO... : remaining types (for tail)
  template<typename O,typename... OO>
  struct Chain<O,OO...> {
    /// introspection
    using Types=Chain<O,OO...>;
    /// chain Head
    using Head=O;
    /// chain tail
    using Tail=Chain<OO...>;
    /// chain size
    static constexpr const SizeT size{1+sizeof...(OO)};
    /// append
    template<typename... XX> using App=Chain<XX...,O,OO...>;
    /// insert
    template<typename... XX> using Ins=Chain<O,OO...,XX...>;
    /// map a transformation over the chain
    template<template<typename> class M> using Map=Chain<M<O>,M<OO>...>;

    // @brief collapse types O,OO... into a single c++ object forming an inheritance chain overt it's internal class `Part<>`
    // @tparam T : the termination object
    template<typename T>
    struct Part:          O::template Part<typename Chain<OO...>::template Part<T>> {
      using Base=typename O::template Part<typename Chain<OO...>::template Part<T>>;
      using Base::Base;
      using Types=Chain<O,OO...>;
    };
  };

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

  // ====================== RULES DETECTION ======================--

  template<typename T, typename = void>
  struct HasRules : std::false_type {};

  template<typename T>
  struct HasRules<T, std::void_t<decltype(T::template rules<void,void>())>> 
    : std::true_type {};

  // ====================== BEFORE / AFTER WALK ======================--

  // default case, target has no rules, call next valid rules, 
  // in practice only the last level match this case (if not having rules itself)
  template<typename Current, typename Before, typename After, bool=HasRules<Current>::value>
  struct RuleLayer {
    template<typename O> struct Part : O {using O::rules;};
  };

  /// @brief rules fold/collapse utility, compose all rules into a single object
  /// @tparam Current : the target object for rule inspection
  /// @tparam Before : chain of elements before the current
  /// @tparam After : chain of elements after the current
  /// @tparam bool 
  template<typename Current, typename Before, typename After>
  struct RuleLayer<Current, Before, After, true> {
    template<typename O>
    struct Part : O {
      static constexpr bool rules() {
        return Current::template rules<Before,After>() && O::rules();
      }
    };
  };

  /// @brief start the rules folding process and walk the list of types
  /// to provide correct before/after elements to each target element in chain
  /// @tparam Before : elements before, starts empty (usually)
  /// @tparam After : elements after, starts with a complete set of all elements to be checked
  template<typename Before, typename After>
  struct BuildRules:
    RuleLayer<typename After::Head,Before,typename After::Tail>::template Part<
      hapi::BuildRules<typename Before::template App<typename After::Head>, typename After::Tail>
    >
  {};

  //rules fold termination
  template<typename Before>
  struct BuildRules<Before,Chain<>> {
    static constexpr bool rules() {return true;}
  };

  // ====================== APIOf ======================--

  /// @brief Close chain composition with a fallback API, 
  /// collapsing the chain into a single c++ class inheritance
  /// that ultimately derive from the given API
  /// @tparam API : fall-back API
  /// @tparam OO... : the chain components 
  template<typename API, typename... OO>
  struct APIOf : Chain<OO...>::template Part<API> {
    using Base = typename Chain<OO...>::template Part<API>;
    using Base::Base;
    static_assert(BuildRules<Chain<>,Chain<OO...>>::rules(), "HAPI: validation failed");
  };

  //empty case, just the fall-back
  template<typename API>
  struct APIOf<API> : API { 
    using Base = API; 
    using Base::Base;
  };

  // Map specialization to traverse directly through an APIOf boundary
  template<typename F, typename API, typename... OO>
  struct Map<F, APIOf<API, OO...>> {
    using Expr = APIOf<API, typename Map<F, OO>::Expr...>;
  };

  /// @brief provide circular reference to the whole chain if needed
  /// optional, use only if your API needs that any component can 
  /// call upper/derived object members.
  /// @note: this will cause your error messages to be even bigger and can lead to loops if not properly used.
  /// @tparam O the final collapsed object complete type
  template<typename O>
  struct CRTP {
    using Obj=O;
    O& obj() {return static_cast<O&>(*this);}
    const O& obj() const {return static_cast<const O&>(*this);}
    
    O* operator->() {return static_cast<O*>(this);}
    const O* operator->() const {return static_cast<const O*>(this);}
  };

};//namespace hapi
