/**
 * @file meta.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi introspection filter and transformations
*/

#pragma once
// #include "hapi/chain.h"

namespace hapi {

  template<typename... OO> struct Chain;

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

  /// @brief predicate: true if O publicly inherits Tag (outer struct declares `struct MyComp : Tag`)
  template<typename Tag>
  struct TagIs {
    template<typename O>
    struct Check : std::is_base_of<Tag, O> {};
  };

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

  // Recurse into nested Chain elements — mirrors Map<F, Chain<OO...>> pattern
  template<typename Q, typename... XX>
  constexpr bool query<Q, Chain<XX...>> = (query<Q, XX> || ...);

  template<typename T, typename=void>
  struct has_type : std::false_type {};
  template<typename T>
  struct has_type<T, std::void_t<typename T::type>> : std::true_type {};

  template<typename Q, typename=void>
  struct is_predicate : std::false_type {};
  template<typename Q>
  struct is_predicate<Q, std::void_t<decltype(Q::template Check<void>::value)>> : std::true_type {};

  // FindFirst<Q, Chain<OO...>, API>:
  //   Searches component list Chain<OO...> for Q; returns the collapsed
  //   O::Part<Tail::Part<API>> base type (valid for static_cast).
  //   SFINAE on Enable halves recursion depth vs Pick<bool>: 1 frame per
  //   non-matching step instead of 2, so peak depth is O(N) not O(2N).
  template<typename Q, typename C, typename API, typename Enable = void>
  struct FindFirst;

  template<typename Q, typename API, typename Enable>
  struct FindFirst<Q, Chain<>, API, Enable> {};

  // Head is a nested Chain, Q matches something inside — recurse in
  template<typename Q, typename... Inner, typename... OO, typename API>
  struct FindFirst<Q, Chain<Chain<Inner...>, OO...>, API,
                   std::enable_if_t<query<Q, Chain<Inner...>>>> {
    using type = typename FindFirst<Q, Chain<Inner...>,
                                    typename Chain<OO...>::template Part<API>>::type;
  };

  // Head is a nested Chain, Q matches nothing inside — skip
  template<typename Q, typename... Inner, typename... OO, typename API>
  struct FindFirst<Q, Chain<Chain<Inner...>, OO...>, API,
                   std::enable_if_t<!query<Q, Chain<Inner...>>>> {
    using type = typename FindFirst<Q, Chain<OO...>, API>::type;
  };

  // Head matches — compute Part only here, not at every level
  template<typename Q, typename O, typename... OO, typename API>
  struct FindFirst<Q, Chain<O, OO...>, API,
                   std::enable_if_t<Q::template Check<O>::value>> {
    using type = typename O::template Part<typename Chain<OO...>::template Part<API>>;
  };

  // Head doesn't match — recurse, no Part instantiated
  template<typename Q, typename O, typename... OO, typename API>
  struct FindFirst<Q, Chain<O, OO...>, API,
                   std::enable_if_t<!Q::template Check<O>::value>> {
    using type = typename FindFirst<Q, Chain<OO...>, API>::type;
  };

  /// @brief find: locate the first component satisfying Q and return a reference to its
  ///        collapsed Part<...> base — valid for static_cast in the mono_block topology.
  ///        Node::Types = Chain<API, OO...>; searches OO... (API is the terminal, not a component).
  template<typename Q, typename Node>
  constexpr auto& find(Node& node) noexcept {
    using FullTypes = typename Node::Types;
    using API       = typename FullTypes::Head;
    using Hapis     = typename FullTypes::Tail;
    // Guard: Node::Types::Head is the API terminal only when Node is an APIOf node.
    // A raw Chain<A,B>::Part<X> exposes A as Head, which find silently skips —
    // making A permanently unreachable. The structural signature of a valid APIOf
    // node is that it inherits from Tail::Part<Head> (the collapsed chain with Head
    // as the API base), which raw Chain::Part never satisfies in that orientation.
    static_assert(
      std::is_base_of_v<typename Hapis::template Part<API>, Node>,
      "find<>: Node must be an APIOf node — "
      "Node::Types::Head is treated as the API terminal (not a component). "
      "Calling find on a raw Chain::Part silently skips Head and can never locate it.");
    static_assert(query<Q, Hapis>, "find<>: no component in chain satisfies predicate Q");
    using FF = FindFirst<Q, Hapis, API>;
    static_assert(has_type<FF>::value, "find<>: component matched by query but not reachable via inheritance — use findBody<> for body items");
    using Found = typename FF::type;
    return static_cast<Found&>(node);
  }

  template<typename Q, typename Node>
  constexpr const auto& find(const Node& node) noexcept {
    using FullTypes = typename Node::Types;
    using API       = typename FullTypes::Head;
    using Hapis     = typename FullTypes::Tail;
    static_assert(
      std::is_base_of_v<typename Hapis::template Part<API>, Node>,
      "find<>: Node must be an APIOf node — "
      "Node::Types::Head is treated as the API terminal (not a component). "
      "Calling find on a raw Chain::Part silently skips Head and can never locate it.");
    static_assert(query<Q, Hapis>, "find<>: no component in chain satisfies predicate Q");
    using FF = FindFirst<Q, Hapis, API>;
    static_assert(has_type<FF>::value, "find<>: component matched by query but not reachable via inheritance — use findBody<> for body items");
    using Found = typename FF::type;
    return static_cast<const Found&>(node);
  }

};
