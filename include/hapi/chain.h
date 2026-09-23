/**
 * @file chain.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi chain — mono_block topology.
 *        Chain<O,OO...>::Part<T> = O::Part<Chain<OO...>::Part<T>> collapses
 *        to a single inheritance stack, so a Chain is itself usable as one
 *        component inside another Chain.
*/

#pragma once

#include "hapi/meta.h"

namespace hapi {
  /// @brief sentinel empty type
  struct Nil {};

  // ====================== CHAIN ======================--

  template<typename... OO> struct Chain;

  // ── Drop<n>: the chain suffix after skipping its first n elements ──────────
  // Haskell's `drop n xs`; Nth is then just Drop<n>::Head. Kept total: dropping
  // past the end gives Chain<> rather than an error (an out-of-range Head
  // access still fails naturally, since Chain<> has no Head). Recursion is by
  // partial specialization, not std::conditional_t<n==0,...,...>: conditional_t
  // requires both branches to already be valid types, so it would eagerly
  // instantiate the n-1 branch even at n==0, underflowing the unsigned SizeT
  // and never terminating. Checked with static_asserts on host g++ and on
  // avr-gcc 7.3 (no STL).
  template<SizeT n,typename L> struct DropOf {using Type=typename DropOf<n-1,typename L::Tail>::Type;};
  template<typename L>         struct DropOf<0,L>       {using Type=L;};
  template<SizeT n>            struct DropOf<n,Chain<>> {using Type=Chain<>;};
  template<>                   struct DropOf<0,Chain<>> {using Type=Chain<>;};  // disambiguates the two specializations above at n=0,L=Chain<>

  /// Empty chain
  template<>
  struct Chain<> {
    template<typename T>
    using Part = T;  // anchor: no more components, collapse to T
    using Types = Chain<>;
    static constexpr SizeT size{0};
    template<template<typename...> class W> using Build = W<>;
    template<typename... XX> using App = Chain<XX...>;
    template<typename... XX> using Ins = Chain<XX...>;
    template<template<typename> class M> using Map = Chain<>;
    template<SizeT n> using Drop = Chain<>;
  };

  // list of types
  template<typename O, typename... OO>
  struct Chain<O, OO...> {
    using Types = Chain<O, OO...>;
    using Head  = O;
    using Tail  = Chain<OO...>;
    static constexpr SizeT size{1 + sizeof...(OO)};
    template<template<typename...> class W> using Build = W<O, OO...>;
    template<typename... XX> using App = Chain<XX..., O, OO...>;
    template<typename... XX> using Ins = Chain<O, OO..., XX...>;
    template<template<typename> class M> using Map = Chain<M<O>, M<OO>...>;
    template<SizeT n> using Drop = typename DropOf<n,Chain<O,OO...>>::Type;

    // A bare Chain<> used directly (no APIOf) has no validation hook of its
    // own -- name collisions between siblings (same method, different
    // signature -- one silently hides the other) go unchecked unless you
    // opt in yourself: see rules.h's NoCollision, usable standalone in a
    // static_assert at your own composition site.
    template<typename T>
    struct Part : O::template Part<typename Chain<OO...>::template Part<T>> {
      using Base = typename O::template Part<typename Chain<OO...>::template Part<T>>;
      using Base::Base;
      using Types = Chain<O, OO...>;
    };
  };

  // ── Expand<O>: what a container holds, taught once per container ───────────────
  // The structural fact behind every walk that needs to open a container: an
  // ordered Chain<...> of children. Primary = leaf (no Children). Opt-in per
  // EXACT type, deliberately never keyed on ::Types: ::Types is a convention on
  // many unrelated types, and a generic ::Types splice was tried and reverted
  // (see hapi.h, commit 7c5e779) for breaking whole-object Filter<FromTypes<..>>.
  // A derived type (e.g. an ItemDef deriving from APIOf) needs its own entry,
  // one line forwarding to its base's: `struct Expand<D<OO...>> : Expand<B<OO...>> {};`
  //
  // The four policy bits say WHICH walks descend, because today they genuinely
  // differ and unifying the structure must not silently unify the policy:
  //   queried    Any/Exists/query/Requires/Excludes look inside
  //   selected   Filter/Map/Partition look inside (false = the element is taken whole)
  //   validates  BuildRules/NoCollision splice it into the rule walk
  //   searched   FindFirst opens it (after testing the node itself)
  // Every bit defaults to false so an entry lists only what it enables.
  template<typename Kids, bool Queried=false, bool Selected=false, bool Validates=false, bool Searched=false>
  struct Expansion {
    using Children = Kids;
    static constexpr bool queried   = Queried;
    static constexpr bool selected  = Selected;
    static constexpr bool validates = Validates;
    static constexpr bool searched  = Searched;
  };

  template<typename O> struct Expand {};   // leaf

  // a plain Chain is transparent: every walk goes through it
  template<typename... OO>
  struct Expand<Chain<OO...>> : Expansion<Chain<OO...>,true,true,true,true> {};

  template<typename O, typename = void> struct IsContainer : std::false_type {};
  template<typename O>
  struct IsContainer<O, std::void_t<typename Expand<O>::Children>> : std::true_type {};

  /// @brief provide circular reference to the whole chain if needed
  template<typename O>
  struct CRTP {
    using Obj=O;
    [[nodiscard]] O& obj() {return static_cast<O&>(*this);}
    [[nodiscard]] const O& obj() const {return static_cast<const O&>(*this);}
    [[nodiscard]] O* operator->() {return static_cast<O*>(this);}
    [[nodiscard]] const O* operator->() const {return static_cast<const O*>(this);}
  };

}; // namespace hapi
