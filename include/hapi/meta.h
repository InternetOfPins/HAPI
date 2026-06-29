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

  struct Nothing {};
  template<typename O> struct Just  { using Type=O; };

  template<typename O> constexpr bool isJust{false};
  template<typename O> constexpr bool isJust<Just<O>>{true};

  template<int V> struct Tag {
    static constexpr int value{V};
    template<typename O> struct Part : O { using O::O; };
  };

  // ── Traverse ──────────────────────────────────────────────────────────────────--

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

  // ── Map ops ───────────────────────────────────────────────────────────────────--

  template<typename Q>
  struct SameAs {
    template<typename O>     using Apply     = std::is_same<Q,O>;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  template<typename Q>
  struct Not {
    template<typename O> using Apply= std::bool_constant<!Q::template Apply<O>::value>;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  template<typename Q,typename R>
  struct Or {
    template<typename O> using Apply= 
      std::bool_constant<Q::template Apply<O>::value||R::template Apply<O>::value>;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  template<typename Q,typename R>
  struct And {
    template<typename O> using Apply= 
      std::bool_constant<Q::template Apply<O>::value&&R::template Apply<O>::value>;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  // ── Transform ─────────────────────────────────────────────────────────────────--

  template<template<typename> class F>
  struct TransformWith {
    template<typename O>     using Apply     = typename F<O>::Type;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  template<template<typename> class F, typename Input>
  using Transform = Eval<TransformWith<F>, Input>;

  // example: Tag<i> -> Tag<i+1>
  template<typename T> struct Succ;
  template<int i> struct Succ<Tag<i>> { using Type = Tag<i+1>; };

  // ── MaybeFirst ────────────────────────────────────────────────────────────────--

  template<typename... OO> struct MaybeFirst_;
  template<> struct MaybeFirst_<>                { using type=Nothing; };
  template<typename H, typename... TT>
  struct MaybeFirst_<H,TT...> : std::conditional_t<
    isJust<H>,
    MaybeFirst_<H>,
    MaybeFirst_<TT...>> {};
  template<typename H> struct MaybeFirst_<H>     { using type=H; };

  template<typename... OO> using MaybeFirst = typename MaybeFirst_<OO...>::type;

  // ── Fold ops ──────────────────────────────────────────────────────────────────--

  template<typename MapOp>
  struct AnyWith {
    template<typename O>     using Apply     = typename MapOp::template Apply<O>;
    template<typename... OO> using ApplyPack = std::bool_constant<(OO::value||...)>;
  };

  template<typename MapOp>
  struct AllWith {
    template<typename O>     using Apply     = typename MapOp::template Apply<O>;
    template<typename... OO> using ApplyPack = std::bool_constant<(OO::value&&...)>;
  };

  template<typename MapOp>
  struct FindFirstWith {
    template<typename O>
    using Apply = std::conditional_t<MapOp::template Apply<O>::value, Just<O>, Nothing>;
    template<typename... OO> using ApplyPack = MaybeFirst<OO...>;
  };

  // ── Sugar ─────────────────────────────────────────────────────────────────────--

  template<typename MapOp, typename Input> using Any       = Eval<AnyWith<MapOp>,       Input>;
  template<typename MapOp, typename Input> using All       = Eval<AllWith<MapOp>,        Input>;
  template<typename MapOp, typename Input> using FindFirst = Eval<FindFirstWith<MapOp>, Input>;

  template<typename Q, typename O>
  constexpr bool query = isJust<FindFirst<Q,O>>;

};
