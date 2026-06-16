/**
 * @file chain.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi chain — mono_block topology
 *        Hapi<T>::Part<O> : T::Part<O>  (delegates through T's collapse)
 *        Chain<OO...> inherits Hapi<Chain<OO...>> — Chain IS a component.
 *        Chain still defines its own Part<T> so Hapi's Part has a non-circular target.
*/

#pragma once

#include "hapi/meta.h"

namespace hapi {
  /// @brief sentinel empty type
  struct Nil {};

  // ====================== COMPONENT ======================--

  /// @brief Wrap T as a component: Part<O> delegates through T::Part<O>.
  ///        Derived from Hapi<T> can delete or hide methods to create restricted views.
  // template<typename T>
  // struct Hapi {
  //   // template<typename O>
  //   // struct Part : T::template Part<O> {
  //   //   using Base = typename T::template Part<O>;
  //   //   using Base::Base;
  //   // };
  // };

  // ====================== CHAIN ======================--

  template<typename... OO> struct Chain;

  // Empty chain — anchors recursion: Part<T> : T
  // Also inherits Hapi<Chain<>> so Chain<> is usable as a component.
  // Own Part<T> shadows Hapi's to avoid circularity.
  template<>
  struct Chain<> /*: Hapi<Chain<>>*/ {
    template<typename T>
    struct Part : T { using T::T; };  // anchor: no more components, collapse to T
    using Types = Chain<>;
    static constexpr SizeT size{0};
    template<template<typename...> class W> using Build = W<>;
    template<typename... XX> using App = Chain<XX...>;
    template<typename... XX> using Ins = Chain<XX...>;
    template<template<typename> class M> using Map = Chain<>;
  };

  /// @brief Non-empty chain.
  ///        - Inherits Hapi<Chain<O,OO...>>: makes Chain usable as a component.
  ///        - Defines own Part<T>: standard collapse O::Part<Chain<OO...>::Part<T>>.
  ///          Own Part shadows Hapi's, so Hapi<Chain<O,OO...>>::Part<T>
  ///          delegates here without circularity.
  template<typename O, typename... OO>
  struct Chain<O, OO...> /*: Hapi<Chain<O, OO...>>*/ {
    using Types = Chain<O, OO...>;
    using Head  = O;
    using Tail  = Chain<OO...>;
    static constexpr SizeT size{1 + sizeof...(OO)};
    template<template<typename...> class W> using Build = W<O, OO...>;
    template<typename... XX> using App = Chain<XX..., O, OO...>;
    template<typename... XX> using Ins = Chain<O, OO..., XX...>;
    template<template<typename> class M> using Map = Chain<M<O>, M<OO>...>;

    template<typename T>
    struct Part : O::template Part<typename Chain<OO...>::template Part<T>> {
      using Base = typename O::template Part<typename Chain<OO...>::template Part<T>>;
      using Base::Base;
      using Types = Chain<O, OO...>;
    };
  };

  template<typename F, typename... OO>
  struct Map<F, Chain<OO...>> {
    using Expr = Chain<typename Map<F, OO>::Expr...>;
  };

  // forEach<Q>(node, fn): call fn on every Q-matching component in node's chain.
  // Mirrors FindFirst but visits all matches. Threads API suffix so each cast is exact —
  // O(N) instantiation depth, zero runtime overhead (all static_cast).

  template<typename Q, typename API, typename Node, typename Fn>
  void forEachIn(Chain<>, Node&, Fn&&) {}

  template<typename Q, typename API, typename Node, typename Fn>
  void forEachIn(Chain<>, const Node&, Fn&&) {}

  template<typename Q, typename API, typename... Inner, typename... Rest, typename Node, typename Fn>
  void forEachIn(Chain<Chain<Inner...>, Rest...>, Node& node, Fn&& fn) {
    forEachIn<Q, typename Chain<Rest...>::template Part<API>>(Chain<Inner...>{}, node, fn);
    forEachIn<Q, API>(Chain<Rest...>{}, node, fn);
  }

  template<typename Q, typename API, typename... Inner, typename... Rest, typename Node, typename Fn>
  void forEachIn(Chain<Chain<Inner...>, Rest...>, const Node& node, Fn&& fn) {
    forEachIn<Q, typename Chain<Rest...>::template Part<API>>(Chain<Inner...>{}, node, fn);
    forEachIn<Q, API>(Chain<Rest...>{}, node, fn);
  }

  template<typename Q, typename API, typename O, typename... Rest, typename Node, typename Fn>
  void forEachIn(Chain<O, Rest...>, Node& node, Fn&& fn) {
    if constexpr (Q::template Check<O>::value) {
      using Found = typename O::template Part<typename Chain<Rest...>::template Part<API>>;
      fn(static_cast<Found&>(node));
    }
    forEachIn<Q, API>(Chain<Rest...>{}, node, fn);
  }

  template<typename Q, typename API, typename O, typename... Rest, typename Node, typename Fn>
  void forEachIn(Chain<O, Rest...>, const Node& node, Fn&& fn) {
    if constexpr (Q::template Check<O>::value) {
      using Found = typename O::template Part<typename Chain<Rest...>::template Part<API>>;
      fn(static_cast<const Found&>(node));
    }
    forEachIn<Q, API>(Chain<Rest...>{}, node, fn);
  }

  template<typename Q, typename Node, typename Fn>
  void forEach(Node& node, Fn&& fn) {
    using API   = typename Node::Types::Head;
    using Hapis = typename Node::Types::Tail;
    forEachIn<Q, API>(Hapis{}, node, fn);
  }

  template<typename Q, typename Node, typename Fn>
  void forEach(const Node& node, Fn&& fn) {
    using API   = typename Node::Types::Head;
    using Hapis = typename Node::Types::Tail;
    forEachIn<Q, API>(Hapis{}, node, fn);
  }

  /// @brief provide circular reference to the whole chain if needed
  template<typename O>
  struct CRTP {
    using Obj=O;
    O& obj() {return static_cast<O&>(*this);}
    const O& obj() const {return static_cast<const O&>(*this);}
    O* operator->() {return static_cast<O*>(this);}
    const O* operator->() const {return static_cast<const O*>(this);}
  };

}; // namespace hapi
