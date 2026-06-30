/**
 * @file meta.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi introspection filter and transformations
*/

#pragma once

#include "hapi/base.h"

//the happy API
namespace hapi {

  template<typename... OO> struct Chain;

  template<typename O> struct Left  { using Type=O; };
  template<typename O> struct Right { using Type=O; };

  template<int V> struct Tag {
    static constexpr int value{V};
    template<typename O> struct Part : O { using O::O; };
  };

  template<typename T> struct Succ;
  template<int V> struct Succ<Tag<V>> { using Type = Tag<V+1>; };

  // ── Traverse: the ONLY container extension point ───────────────────────────────--

  template<typename Op, typename Input> struct Traverse;

  template<typename Op, typename Input>
  using Eval = typename Traverse<Op, Input>::Beta;

  template<typename Op, typename Input>
  struct Traverse {
    using Beta = typename Op::template Apply<Input>;
  };

  template<typename Op, typename... OO>
  struct Traverse<Op, Chain<OO...>> {
    using Beta = typename Op::template ApplyPack<typename Traverse<Op, OO>::Beta...>;
  };

  // ── Predicates ───────────────────────────────────────────────────────────────--

  template<typename Q>
  struct SameAs {
    template<typename O> using Check    = typename Traverse<SameAs<Q>,O>::Beta;
    template<typename O> using Apply    = std::is_same<Q,O>;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  template<typename Tag>
  struct TagIs {
    template<typename O> using Check    = typename Traverse<TagIs<Tag>,O>::Beta;
    template<typename O> using Apply    = std::is_base_of<Tag,O>;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  template<template<typename...> class Wrapper>
  struct IsInstanceOf {
    template<typename O> using Check = typename Traverse<IsInstanceOf<Wrapper>,O>::Beta;
    template<typename O> struct Apply : std::false_type {};
    template<typename... OO> struct Apply<Wrapper<OO...>> : std::true_type {};
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  /// @brief drill into O::Types and apply Q to it; only true if O has ::Types
  template<typename Q>
  struct FromTypes {
    template<typename O, typename = void>
    struct Apply : std::false_type {};  // no ::Types member

    template<typename O>
    struct Apply<O, std::void_t<typename O::Types>>
      : std::bool_constant<Exists<Q, typename O::Types>::value> {};

    template<typename O> using Check = typename Traverse<FromTypes<Q>, O>::Beta;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  template<typename Q,template<typename> class L=Left,template<typename> class R=Right>
  struct Partition {
    template<typename O> using Check    = typename Traverse<Partition<Q>,O>::Beta;
    template<typename O> using Apply = std::conditional_t<Q::template Apply<O>::value, L<O>, R<O>>;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  // ── Predicate combinators ───────────────────────────────────────────────────────--
  // compose at leaf level (Apply); Check/ApplyPack just let these stand in
  // anywhere a predicate is expected — same shape as SameAs.

  template<typename Q>
  struct Not {
    template<typename O> using Check    = typename Traverse<Not<Q>,O>::Beta;
    template<typename O> using Apply    = std::bool_constant<!Q::template Apply<O>::value>;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  template<typename A, typename B>
  struct And {
    template<typename O> using Check    = typename Traverse<And<A,B>,O>::Beta;
    template<typename O> using Apply    = std::bool_constant<
      A::template Apply<O>::value && B::template Apply<O>::value>;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  template<typename A, typename B>
  struct Or {
    template<typename O> using Check    = typename Traverse<Or<A,B>,O>::Beta;
    template<typename O> using Apply    = std::bool_constant<
      A::template Apply<O>::value || B::template Apply<O>::value>;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  // ── Map ─────────────────────────────────────────────────────────────────────────--
  // leaf-level transform F<O>::Type; ApplyPack just rebuilds the Chain shape,
  // so nested Chains map structurally without Map having to know about Chain itself.

  template<template<typename> class F>
  struct Map {
    template<typename O> using Check    = typename Traverse<Map<F>,O>::Beta;
    template<typename O> using Apply    = typename F<O>::Type;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  template<template<typename> class F, typename Input>
  using Transform = Eval<Map<F>, Input>;

  // ── Fold: Any ──────────────────────────────────────────────────────────────────--

  template<typename Q>
  struct Any {
    template<typename O> using Check = typename Traverse<Any<Q>, O>::Beta;
    template<typename O> using Apply = typename Q::template Apply<O>;
    template<typename... OO> using ApplyPack = std::bool_constant<(OO::value || ...)>;
  };

  // ── Filter ─────────────────────────────────────────────────────────────────────--
  // right-fold: splice a pack of fragment-Chains into one, preserving order

  template<typename... Fragments> struct ConcatChains;
  template<> struct ConcatChains<> { using Type = Chain<>; };
  template<typename... OO, typename... Rest>
  struct ConcatChains<Chain<OO...>, Rest...> {
    template<typename... RR> struct Splice;
    template<typename... RR> struct Splice<Chain<RR...>> { using Type = Chain<OO...,RR...>; };
    using Type = typename Splice<typename ConcatChains<Rest...>::Type>::Type;
  };

  template<typename Q>
  struct Filter {
    template<typename O> using Check = typename Traverse<Filter<Q>, O>::Beta;
    template<typename O> using Apply = std::conditional_t<Q::template Check<O>::value,Chain<O>,Chain<>>;
    template<typename... OO> using ApplyPack = typename ConcatChains<OO...>::Type;
  };

  // ── FindFirst ──────────────────────────────────────────────────────────────────--
  // own head/tail walk (not routed through Traverse): stops at the first match,
  // never instantiates a sibling past it. No Just/Nothing — querying a chain with
  // no match is a compile error (dead end has no ::Result member to access).

  template<typename Q, typename Input> struct FindFirst_;

  // detect whether a (possibly nested) search already produced a Result
  template<typename T, typename = void> struct HasResult : std::false_type {};
  template<typename T> struct HasResult<T, std::void_t<typename T::Result>> : std::true_type {};

  // leaf: Result present only if the predicate accepts it
  template<typename Q, typename Input, bool=Q::template Apply<Input>::value>
  struct FindFirstLeaf {}; // miss: no Result member
  template<typename Q, typename Input>
  struct FindFirstLeaf<Q,Input,true> { using Result = Input; };

  template<typename Q, typename Input>
  struct FindFirst_ : FindFirstLeaf<Q,Input> {};

  // chain: dispatch on whether Head's search already has a Result; each branch
  // only names the chain element it actually needs, so the miss-branch's lone
  // reference to Chain<TT...> is the only place that ever instantiates the tail
  template<typename Q, typename HeadSearch, typename Tail, bool=HasResult<HeadSearch>::value>
  struct FindFirstChain : FindFirst_<Q,Tail> {};               // miss: try tail
  template<typename Q, typename HeadSearch, typename Tail>
  struct FindFirstChain<Q,HeadSearch,Tail,true> : HeadSearch {}; // hit: stop here

  template<typename Q, typename H, typename... TT>
  struct FindFirst_<Q, Chain<H,TT...>> : FindFirstChain<Q, FindFirst_<Q,H>, Chain<TT...>> {};

  template<typename Q>
  struct FindFirst_<Q, Chain<>> {}; // dead end: no Result

  template<typename Q>
  struct FindFirst {
    template<typename Input> using Check = typename FindFirst_<Q, Input>::Result;
  };

  // ── Soft-fail variants ─────────────────────────────────────────────────────────--

  /// @brief presence-only check: never fails to compile, just answers true/false.
  /// reuses Any (already a short-circuit-free fold over the whole chain) —
  /// no need to duplicate FindFirst_'s walk for a question that's just bool.
  template<typename Q, typename Input>
  using Exists = typename Any<Q>::template Check<Input>;

  // find-or-default: same head/tail walk as FindFirst, but a miss yields
  // Default instead of a compile error. Still short-circuits via FindFirstChain.
  template<typename Q, typename Default, typename Input, bool=HasResult<FindFirst_<Q,Input>>::value>
  struct FindFirstOrLeaf { using Result = Default; }; // miss: fall back
  template<typename Q, typename Default, typename Input>
  struct FindFirstOrLeaf<Q,Default,Input,true> { using Result = typename FindFirst_<Q,Input>::Result; };

  template<typename Q, typename Default>
  struct FindFirstOr {
    template<typename Input> using Check = typename FindFirstOrLeaf<Q,Default,Input>::Result;
  };

  /// @brief compatibility shim for the rules system (Requires/Excludes/APIOf):
  /// boolean presence of predicate Q anywhere in O, as a constexpr value (not a type).
  template<typename Q, typename O>
  constexpr bool query = Exists<Q,O>::value;

  // ── At<N> ──────────────────────────────────────────────────────────────────────--

  template<std::size_t idx,typename O>
  struct At {
    using Type=std::conditional_t<(idx>0),typename hapi::At<idx-1,typename O::Base>::Type,O>;
  };

  template<std::size_t idx,auto ref>
  constexpr auto& at() {return static_cast<typename At<idx,decltype(ref)>::Type&>(ref);}

  // ── Runtime query functions ────────────────────────────────────────────────────--

  /// @brief find first match of Q in C's ::Types chain, return typed reference into C
  template<typename Q, typename C>
  decltype(auto) find(C& c) {
    using Found = typename FindFirst<Q>::template Check<typename C::Types>;
    return static_cast<typename C::template Part<Found>&>(c);
  }

  template<typename Q, typename C>
  decltype(auto) find(const C& c) {
    using Found = typename FindFirst<Q>::template Check<typename C::Types>;
    return static_cast<const typename C::template Part<Found>&>(c);
  }

  /// @brief find match of Q in C's ::Types chain via default if miss, return typed reference
  template<typename Q, typename Default, typename C>
  decltype(auto) findOr(C& c) {
    using Found = typename FindFirstOr<Q, Default>::template Check<typename C::Types>;
    return static_cast<typename C::template Part<Found>&>(c);
  }

  template<typename Q, typename Default, typename C>
  decltype(auto) findOr(const C& c) {
    using Found = typename FindFirstOr<Q, Default>::template Check<typename C::Types>;
    return static_cast<const typename C::template Part<Found>&>(c);
  }

};
