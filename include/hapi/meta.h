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

  // ── Expand<O>: what a container holds, taught once per container ───────────────
  // The structural fact behind every walk that needs to open a container: an
  // ordered Chain<...> of children. Primary = leaf: declared but never defined, so
  // asking about a leaf instantiates nothing (the walks probe every element). Opt-in per
  // EXACT type, deliberately never keyed on ::Types: ::Types is a convention on
  // many unrelated types, and a generic ::Types splice was tried and reverted
  // (HAPI commit 7c5e779) for breaking whole-object Filter<FromTypes<..>> in
  // OneMenu. A derived type (e.g. an ItemDef deriving from APIOf) needs its own
  // entry, one line forwarding to its base's:
  //   template<typename... OO> struct Expand<D<OO...>> : Expand<B<OO...>> {};
  // and, like any specialization, it must be declared before Expand is first
  // used on that type.
  //
  // The four policy bits say WHICH walks descend, because they genuinely differ
  // and unifying the structure must not silently unify the policy:
  //   queried    Any/Exists/query/Requires/Excludes look inside (ops with is_query)
  //   selected   every other Traverse op (Filter/Map/Partition/...) looks inside;
  //              false = the element is taken whole
  //   validates  BuildRules/NoCollision splice it into the rule walk
  //   searched   FindFirst opens it, after testing the node itself
  // Every bit defaults to false so an entry lists only what it enables.
  template<typename Kids, bool Queried=false, bool Selected=false, bool Validates=false, bool Searched=false>
  struct Expansion {
    using Children = Kids;
    static constexpr bool queried   = Queried;
    static constexpr bool selected  = Selected;
    static constexpr bool validates = Validates;
    static constexpr bool searched  = Searched;
  };

  template<typename O> struct Expand;   // leaf: declared, never defined, so probing a leaf instantiates nothing

  // a plain Chain is transparent: every walk goes through it
  template<typename... OO>
  struct Expand<Chain<OO...>> : Expansion<Chain<OO...>,true,true,true,true> {};

  template<typename O, typename = void> struct IsContainer : std::false_type {};
  template<typename O>
  struct IsContainer<O, std::void_t<typename Expand<O>::Children>> : std::true_type {};

  // per-walk lookups: false for a leaf, else the container's own bit
  // one SFINAE probe of Expand<O> each (no IsContainer layer): for a leaf the probe instantiates no class
  template<typename O, typename = void> struct Validates : std::false_type {};
  template<typename O> struct Validates<O, std::void_t<typename Expand<O>::Children>> : std::bool_constant<Expand<O>::validates> {};
  template<typename O, typename = void> struct Searches : std::false_type {};
  template<typename O> struct Searches<O, std::void_t<typename Expand<O>::Children>> : std::bool_constant<Expand<O>::searched> {};

  // is the first element of a list one the rule walks splice in place? (false for an empty list)
  template<typename L, typename = void> struct HeadValidates : std::false_type {};
  template<typename H, typename... TT>
  struct HeadValidates<Chain<H,TT...>, std::void_t<typename Expand<H>::Children>> : std::bool_constant<Expand<H>::validates> {};

  // Traverse ops: a query op (is_query, i.e. Any) uses `queried`, every other op uses `selected`
  template<typename Op, typename = void> struct IsQueryOp : std::false_type {};
  template<typename Op> struct IsQueryOp<Op, std::void_t<decltype(Op::is_query)>> : std::bool_constant<Op::is_query> {};
  template<typename Op, typename O, typename = void> struct Opens : std::false_type {};
  template<typename Op, typename O>
  struct Opens<Op,O, std::void_t<typename Expand<O>::Children>> : std::bool_constant<IsQueryOp<Op>::value ? Expand<O>::queried : Expand<O>::selected> {};

  // ── Traverse: the ONLY container extension point ───────────────────────────────--

  template<typename Op, typename Input> struct Traverse;

  template<typename Op, typename Input>
  using Eval = typename Traverse<Op, Input>::Beta;

  // a leaf, or a container this Op opens (Expand<Input>::queried/selected): apply Op per child, then combine
  template<typename Op, typename Kids> struct TraverseKids;
  template<typename Op, typename... OO>
  struct TraverseKids<Op, Chain<OO...>> {
    using Beta = typename Op::template ApplyPack<typename Traverse<Op, OO>::Beta...>;
  };
  // the two branches are alias templates inside a class per bool, not a class per (Op,Input): only the branch taken
  // is substituted, and a leaf costs no extra class instantiation beyond Traverse and Opens themselves
  template<bool Open> struct TraverseBeta;
  template<> struct TraverseBeta<false> { template<typename Op, typename Input> using Of = typename Op::template Apply<Input>; };
  template<> struct TraverseBeta<true>  { template<typename Op, typename Input> using Of = typename TraverseKids<Op, typename Expand<Input>::Children>::Beta; };

  // the extension point is unchanged: a type may still specialize Traverse<Op,X<...>> by hand,
  // and that specialization wins over this primary.
  template<typename Op, typename Input>
  struct Traverse { using Beta = typename TraverseBeta<Opens<Op,Input>::value>::template Of<Op,Input>; };

  // fast path for the container every walk descends (= Expand<Chain>: all bits on), no lookups
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
  // Recursive, not a fold expression: MSVC rejects a unary fold over a
  // dependent pack member (e.g. (OO::value || ...)). Not std::disjunction --
  // AVR's <type_traits> shim (avr_std.h) only provides bool_constant.

  template<typename... OO> struct OrPack;
  template<> struct OrPack<> : std::false_type {};
  template<typename O, typename... OO>
  struct OrPack<O, OO...> : std::bool_constant<O::value || OrPack<OO...>::value> {};

  template<typename Q>
  struct Any {
    static constexpr bool is_query = true;   // opens containers by their `queried` bit
    template<typename O> using Check = typename Traverse<Any<Q>, O>::Beta;
    template<typename O> using Apply = typename Q::template Apply<O>;
    template<typename... OO> using ApplyPack = OrPack<OO...>;
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

  // A container that opts in (Expand<Input>::searched) is tested as a whole FIRST and only then opened:
  // Q may want the container itself (agnosticism's refid matches a whole APIOf cell through FromTypes).
  // Chain is not handled here, it is the walk itself (specializations below) and is never matched as an element.
  template<typename Q, typename Input, bool Hit>
  struct FindFirstOpen : FindFirst_<Q, typename Expand<Input>::Children> {};   // miss: open it
  template<typename Q, typename Input>
  struct FindFirstOpen<Q,Input,true> : FindFirstLeaf<Q,Input> {};              // hit: the node itself

  // leaf, or a container that is not searched: just the leaf test; a searched container: node first, then open it
  template<bool Search> struct FindFirstWith;
  template<> struct FindFirstWith<false> { template<typename Q, typename Input> using Of = FindFirstLeaf<Q,Input>; };
  template<> struct FindFirstWith<true>  { template<typename Q, typename Input> using Of = FindFirstOpen<Q,Input,HasResult<FindFirstLeaf<Q,Input>>::value>; };

  template<typename Q, typename Input>
  struct FindFirst_ : FindFirstWith<Searches<Input>::value>::template Of<Q,Input> {};

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

  /// @brief compatibility shim for the rules system (Requires/Excludes/APIOf):
  /// boolean presence of predicate Q anywhere in O, as a constexpr value (not a type).
  template<typename Q, typename O>
  constexpr bool query = Exists<Q,O>::value;

  // ── At<N> ──────────────────────────────────────────────────────────────────────--
  // IsConst is threaded separately (not read off O at each step) because
  // O::Base is looked up on the possibly-const O but a nested typedef name
  // never itself carries constness -- without this, at() on a const object
  // silently loses const one level up and static_cast then fails to compile
  // ("casting away const") instead of just working.

  template<std::size_t idx,typename O,bool IsConst=std::is_const<O>::value>
  struct At {
    using Type=typename hapi::At<idx-1,typename std::remove_const_t<O>::Base,IsConst>::Type;
  };
  template<typename O,bool IsConst>
  struct At<0,O,IsConst> { using Type=std::conditional_t<IsConst,const O,O>; };

  // Three overloads instead of the old auto-NTTP form (template<auto ref>):
  // a class-type object bound by value as a non-type template parameter is
  // C++20-only (P1907) and this project stays on C++17, so the NTTP form
  // could never actually be called. Ordinary function parameters have no
  // such restriction and additionally work on locals/temporaries, which a
  // linkage-requiring NTTP never could either way.
  template<std::size_t idx,typename O>
  [[nodiscard]] constexpr auto& at(O& obj) {return static_cast<typename At<idx,O>::Type&>(obj);}

  template<std::size_t idx,typename O>
  [[nodiscard]] constexpr auto& at(O* obj) {return at<idx>(*obj);}

  /// @brief for a temporary: bind the result to a named reference first
  /// (e.g. `auto&& tmp=T{}; at<idx>(tmp);`) -- otherwise the returned
  /// reference dangles past the end of the full expression, same as any
  /// function returning a reference into an rvalue argument.
  template<std::size_t idx,typename O>
  [[nodiscard]] constexpr auto&& at(O&& obj) {return static_cast<typename At<idx,O>::Type&&>(obj);}

  // ── Runtime query functions ────────────────────────────────────────────────────--

  /// @brief find first match of Q in C's ::Types: verify Q matches, return full object ref.
  template<typename Q, typename C>
  [[nodiscard]] decltype(auto) find(C& c) {
    using Types = typename C::Types;
    static_assert(HasResult<FindFirst_<Q, Types>>::value, "find: predicate Q not found in C::Types");
    return c;
  }

};
