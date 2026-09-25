/**
 * @file rules.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi assembly chain validation
*/

#pragma once
#include "hapi/chain.h"
#include "hapi/meta.h"

namespace hapi {
  /// @brief true if predicate X matches at least one element in any of Chains.
  /// Pass After only for directional checks; pass Before,After for full-chain checks.
  template<typename X, typename... Chains>
  inline constexpr bool Requires = []() {
    static_assert(sizeof...(Chains) > 0, "Requires<X>: no chain specified — pass After, or Before+After for full-chain check");
    return (query<X, Chains> || ...);
  }();

  /// @brief true if predicate X matches no element in any of Chains.
  /// Pass After only for directional checks; pass Before,After for full-chain checks.
  template<typename X, typename... Chains>
  inline constexpr bool Excludes = []() {
    static_assert(sizeof...(Chains) > 0, "Excludes<X>: no chain specified — pass After, or Before+After for full-chain check");
    return (!query<X, Chains> && ...);
  }();
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

  /// @brief rules fold/collapse utility, compose all rules into a single object.
  template<typename Current, typename Before, typename After>
  struct RuleLayer<Current, Before, After, true> {
    template<typename O>
    struct Part : O {
      [[nodiscard]] static constexpr bool rules() {
        return Current::template rules<Before,After>() && O::rules();
      }
    };
  };

  /// @brief starts the rules folding process, walking the list of types to provide
  /// correct before/after elements to each target element in the chain. A head that
  /// is a container with `validates` (Chain, APIOf, ...) is first replaced by its
  /// children in place, so ITS elements' rules see the right Before/After context.
  template<typename Before, typename After, bool = HeadValidates<After>::value>
  struct BuildRules;

  template<typename Before, typename After>
  struct BuildRules<Before, After, false>:
    RuleLayer<typename After::Head,Before,typename After::Tail>::template Part<
      hapi::BuildRules<typename Before::template App<typename After::Head>, typename After::Tail>
    >
  {};

  //rules fold termination
  template<typename Before>
  struct BuildRules<Before,Chain<>,false> {
    [[nodiscard]] static constexpr bool rules() {return true;}
  };

  /// @brief does a container that is spliced into the rule walk carry a rules() of its own? Chain and APIOf never do,
  /// and must not be probed: HasRules needs a complete type, and for a nested APIOf that would instantiate the whole
  /// composed class (and fire its own validation as a hard error instead of letting the walk report false). Any other
  /// container is asked (e.g. a printer wrapper that has a rule of its own).
  template<typename O> struct HasOwnRules : HasRules<O> {};
  template<typename... OO> struct HasOwnRules<Chain<OO...>> : std::false_type {};

  // The container's OWN rules() (if it has any) still runs, with the same Before/After it would see if placed directly
  // (its later siblings, not its own children): only then is it replaced by its children. Dropping it would silently
  // switch off a rule that is live when the same component is placed directly.
  template<typename Before, typename After>
  struct BuildRules<Before, After, true>
    : RuleLayer<typename After::Head, Before, typename After::Tail, HasOwnRules<typename After::Head>::value>::template Part<
        BuildRules<Before, typename ConcatChains<typename Expand<typename After::Head>::Children,
                                                 typename After::Tail>::Type>
      > {};

  // ====================== MEMBER COLLISION DETECTION ======================--
  // Ordinary C++ name lookup silently hides one same-named method behind
  // another when two Chain<> siblings declare it with different signatures
  // (found for real in .RnD/focCompose: Sensor's void init() vs. Driver's
  // int init(), folded into one Chain -- only the driver's stayed reachable,
  // no error, no warning). C++17 has no reflection over member names, so
  // this can't be fully automatic -- decltype(&T::name) needs `name`
  // spelled literally by whoever already knows it matters (the API/contract
  // author, same "mirror the real names" convention APIOf consumers already
  // follow). Two accepted, documented limitations, not silently swallowed:
  // (1) an overloaded name on the probed type makes &T::name ill-formed, so
  // Has<T> reports false -- a silent miss, not a false positive; (2)
  // identical signatures on both sides are never flagged -- no behavioral
  // surprise, out of scope by design.

  /// @brief opt-in per-member-name detector, invoked once per hazardous
  /// name -- same void_t presence-detection shape as HasRules/HasResult.
  #define HAPI_DETECT_MEMBER(name) \
    struct HapiMember_##name { \
      template<typename T, typename = void> \
      struct Has : std::false_type {}; \
      template<typename T> \
      struct Has<T, std::void_t<decltype(&T::name)>> : std::true_type {}; \
      template<typename T> using Sig = decltype(&T::name); \
    }

  /// @brief a component in the "O-position" of a Chain (has its own nested
  /// Part<T>, e.g. BLDCDriver3PWM) contributes members via
  /// O::template Part<Nil>; a terminal/API type (no nested Part<T>, e.g.
  /// SensorAPI) contributes directly.
  template<typename O, typename = void>
  struct HasPart : std::false_type {};
  template<typename O>
  struct HasPart<O, std::void_t<typename O::template Part<Nil>>> : std::true_type {};

  template<typename O, bool = HasPart<O>::value>
  struct MemberScope { using Type = O; };
  template<typename O>
  struct MemberScope<O, true> { using Type = typename O::template Part<Nil>; };

  /// @brief fires a legible static_assert naming Detector/A/B directly in
  /// the compiler's "required from" backtrace instead of generic template
  /// noise. The condition is template-parameter-dependent (never literally
  /// `false`), so it only fires once this exact specialization is
  /// instantiated -- the standard "dependent false" idiom.
  template<typename Detector, typename A, typename B, bool Collide>
  struct MemberCollision : std::true_type {};
  template<typename Detector, typename A, typename B>
  struct MemberCollision<Detector, A, B, true> {
    static_assert(!sizeof(Detector*),
      "HAPI: member collision -- two composed types provide the same "
      "member with different signatures, so one silently hides the other "
      "via ordinary C++ name lookup. See this MemberCollision<Detector,A,B> "
      "instantiation for which member (Detector) and which two types.");
    static constexpr bool value = false;
  };

  template<typename Detector, typename A, typename B, bool BothPresent>
  struct SigDiffers : std::false_type {};
  template<typename Detector, typename A, typename B>
  struct SigDiffers<Detector,A,B,true> : std::bool_constant<
    !std::is_same<typename Detector::template Sig<A>, typename Detector::template Sig<B>>::value> {};

  template<typename Detector, typename Elem, typename Rest> struct NoCollisionWith_;
  template<typename Detector, typename Elem>
  struct NoCollisionWith_<Detector, Elem, Chain<>> : std::true_type {};
  template<typename Detector, typename Elem, typename O, typename... OO>
  struct NoCollisionWith_<Detector, Elem, Chain<O,OO...>> {
    using SA = typename MemberScope<Elem>::Type;
    using SB = typename MemberScope<O>::Type;
    static constexpr bool bothPresent =
      Detector::template Has<SA>::value && Detector::template Has<SB>::value;
    static constexpr bool ok = MemberCollision<Detector, Elem, O,
      SigDiffers<Detector,SA,SB,bothPresent>::value>::value;
    static constexpr bool value = ok && NoCollisionWith_<Detector, Elem, Chain<OO...>>::value;
  };

  // same in-place splice as BuildRules: a head that is a container with `validates` (nested
  // Chain, nested APIOf, ...) is replaced by its children before the walk continues.
  template<typename Detector, typename Input, bool = HeadValidates<Input>::value>
  struct NoCollision_;
  template<typename Detector>
  struct NoCollision_<Detector, Chain<>, false> : std::true_type {};
  template<typename Detector, typename O, typename... OO>
  struct NoCollision_<Detector, Chain<O,OO...>, false> {
    static constexpr bool value =
      NoCollisionWith_<Detector, O, Chain<OO...>>::value &&
      NoCollision_<Detector, Chain<OO...>>::value;
  };
  template<typename Detector, typename Input>
  struct NoCollision_<Detector, Input, true>
    : NoCollision_<Detector, typename ConcatChains<typename Expand<typename Input::Head>::Children,
                                                   typename Input::Tail>::Type> {};

  /// @brief public entry point, same calling convention as Requires/
  /// Excludes above (direct bool, no ::value) -- usable standalone in a
  /// static_assert at any Chain<> composition site (bare or via APIOf),
  /// or from inside a component's own rules<Before,After>() for APIOf-
  /// based compositions that want it folded in automatically (reconstruct
  /// the full list first via ConcatChains<Before,Chain<Self>,After>).
  template<typename Detector, typename Input>
  inline constexpr bool NoCollision = NoCollision_<Detector, Input>::value;

  // ====================== DISTINCT LAYERS ======================--
  // No layer may occur twice in one composition. Checked on exact types at instantiation, so it sees what a
  // source-level check cannot: pack elements, aliases (Wave<...> vs WaveOf<Slot<...>,...>), equal types spelled
  // differently (Bias<1> vs Bias<0+1>), and types defined in other headers. The list is flattened first:
  //   Chain<...>              spliced: a nested chain is its elements
  //   a type with ::Types     a named composition (a struct over a chain, an APIOf): replaced by its Types, recursively
  //   a type with Part<O>     an open layer: compared by is_same
  //   anything else           a closed operand: compared by is_same, and by is_base_of (either way) with the other
  //                           closed operands, so a closed type and one derived from it do not both appear
  // Usage: static_assert(hapi::Distinct<Chain<A,B,OO...,T>>, "duplicate layer in Z");

  template<typename O, typename = void> struct HasTypes : std::false_type {};
  template<typename O> struct HasTypes<O, std::void_t<typename O::Types>> : std::true_type {};

  template<typename O> struct OpenLayer   { using Type = O; };
  template<typename O> struct ClosedLayer { using Type = O; };

  // 0 open layer, 1 named composition (splice its Types), 2 closed operand, 3 Chain (splice)
  template<typename O> struct LayerKind { static constexpr int value = HasTypes<O>::value ? 1 : HasPart<O>::value ? 0 : 2; };
  template<typename... OO> struct LayerKind<Chain<OO...>> { static constexpr int value = 3; };

  template<typename O, int = LayerKind<O>::value> struct LayersOf_;
  template<typename O> struct LayersOf_<O,0> { using Type = Chain<OpenLayer<O>>; };
  template<typename O> struct LayersOf_<O,1> { using Type = typename LayersOf_<typename O::Types>::Type; };
  template<typename O> struct LayersOf_<O,2> { using Type = Chain<ClosedLayer<O>>; };
  template<typename... OO> struct LayersOf_<Chain<OO...>,3> {
    using Type = typename ConcatChains<typename LayersOf_<OO>::Type...>::Type;
  };
  /// @brief the flattened layer list Distinct compares: Chain<OpenLayer<X>|ClosedLayer<X>...>
  template<typename L> using LayersOf = typename LayersOf_<L>::Type;

  template<typename A, typename B> struct LayerClash : std::false_type {};
  template<typename A, typename B> struct LayerClash<OpenLayer<A>, OpenLayer<B>> : std::is_same<A,B> {};
  template<typename A, typename B> struct LayerClash<ClosedLayer<A>, ClosedLayer<B>>
    : std::bool_constant<std::is_same<A,B>::value || std::is_base_of<A,B>::value || std::is_base_of<B,A>::value> {};

  template<typename E, typename L> struct ClashesWith;
  template<typename E> struct ClashesWith<E, Chain<>> : std::false_type {};
  template<typename E, typename O, typename... OO> struct ClashesWith<E, Chain<O,OO...>>
    : std::bool_constant<LayerClash<E,O>::value || ClashesWith<E, Chain<OO...>>::value> {};

  template<typename L> struct Distinct_;
  template<> struct Distinct_<Chain<>> : std::true_type {};
  template<typename O, typename... OO> struct Distinct_<Chain<O,OO...>>
    : std::bool_constant<!ClashesWith<O, Chain<OO...>>::value && Distinct_<Chain<OO...>>::value> {};

  /// @brief true when no layer of the (flattened) list L occurs twice; same calling convention as Requires/Excludes/NoCollision
  template<typename L>
  inline constexpr bool Distinct = Distinct_<LayersOf<L>>::value;

};
