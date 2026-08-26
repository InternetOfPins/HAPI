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
  /// correct before/after elements to each target element in the chain.
  template<typename Before, typename After>
  struct BuildRules:
    RuleLayer<typename After::Head,Before,typename After::Tail>::template Part<
      hapi::BuildRules<typename Before::template App<typename After::Head>, typename After::Tail>
    >
  {};

  //rules fold termination
  template<typename Before>
  struct BuildRules<Before,Chain<>> {
    [[nodiscard]] static constexpr bool rules() {return true;}
  };

  /// @brief a nested Chain used as one component (mono_block) is spliced
  /// into the walk in place, so its own elements' rules see correct
  /// Before/After context — mirrors Chain<>::Part<T>'s own transparent
  /// handling of a nested Chain as a component. Sibling specialization
  /// for nested APIOf is in hapi.h (kept separate, not a generic
  /// ::Types-keyed splice — see that file for why).
  template<typename Before, typename... PP, typename... Rest>
  struct BuildRules<Before, Chain<Chain<PP...>, Rest...>>
    : BuildRules<Before, Chain<PP..., Rest...>> {};

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

  template<typename Detector, typename Input> struct NoCollision_;
  template<typename Detector>
  struct NoCollision_<Detector, Chain<>> : std::true_type {};
  template<typename Detector, typename O, typename... OO>
  struct NoCollision_<Detector, Chain<O,OO...>> {
    static constexpr bool value =
      NoCollisionWith_<Detector, O, Chain<OO...>>::value &&
      NoCollision_<Detector, Chain<OO...>>::value;
  };
  // mirrors BuildRules's own nested-bare-Chain splice above (mono_block):
  // a Chain used as one component is spliced into the walk in place.
  template<typename Detector, typename... PP, typename... Rest>
  struct NoCollision_<Detector, Chain<Chain<PP...>, Rest...>>
    : NoCollision_<Detector, Chain<PP..., Rest...>> {};

  /// @brief public entry point, same calling convention as Requires/
  /// Excludes above (direct bool, no ::value) -- usable standalone in a
  /// static_assert at any Chain<> composition site (bare or via APIOf),
  /// or from inside a component's own rules<Before,After>() for APIOf-
  /// based compositions that want it folded in automatically (reconstruct
  /// the full list first via ConcatChains<Before,Chain<Self>,After>).
  /// Sibling splice specialization for nested APIOf is in hapi.h, same
  /// reason BuildRules's own APIOf splice lives there and not here.
  template<typename Detector, typename Input>
  inline constexpr bool NoCollision = NoCollision_<Detector, Input>::value;

};
