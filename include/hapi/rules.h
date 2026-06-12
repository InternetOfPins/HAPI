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
  /// With no chains (Chains empty), returns false — caller decides scope.
  template<typename X, typename... Chains>
  inline constexpr bool Requires = (query<X, Chains> || ...);

  /// @brief true if predicate X matches no element in any of Chains.
  /// Pass After only for directional checks; pass Before,After for full-chain checks.
  /// With no chains (Chains empty), returns true — caller decides scope.
  template<typename X, typename... Chains>
  inline constexpr bool Excludes = (!query<X, Chains> && ...);
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

};
