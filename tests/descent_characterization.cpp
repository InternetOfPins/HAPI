/**
 * @file descent_characterization.cpp
 * @brief Pins how HAPI's structural walks treat containers TODAY (compile-only).
 *
 * Written before unifying Traverse / FindFirst / BuildRules / NoCollision behind
 * one "how a container expands" trait, so that refactor can be checked against
 * recorded behaviour instead of assumptions. Every assertion here passes on the
 * current headers. Lines tagged [D2]/[D3] are CURRENT BEHAVIOUR that a later,
 * deliberate policy flip is expected to change: when one of those flips, that
 * assert should change in the same commit, with the reason. Anything else
 * failing after the refactor is a regression.
 *
 * Expect-to-fail counterparts (rule static_asserts firing, diagnostics text)
 * live in tests/negative/ (run tests/negative/run.sh).
 */
#include "../include/hapi/hapi.h"
using namespace hapi;

namespace descent_char {
  struct API {};  struct API2 {};
  // plain components: an APIOf needs each of its components to provide a nested Part
  struct A {template<typename O> struct Part : O {using O::O;};};
  struct B {template<typename O> struct Part : O {using O::O;};};
  struct C {template<typename O> struct Part : O {using O::O;};};

  // A component whose rules() always fails, to see whether a walk reaches it.
  struct BadRule {
    template<typename Bf,typename Af> static constexpr bool rules() {return false;}
    template<typename O> struct Part : O {using O::O;};
  };

  // Stand-in for OneMenu's Menu/StaticBody: wraps other components in its
  // template args and is made visible to Traverse (hence to queries) with its
  // own specialization, but is not spliced by BuildRules/NoCollision.
  template<typename... II> struct W {template<typename O> struct Part : O {using O::O;};};
}
namespace hapi {
  template<typename Op,typename... II>
  struct Traverse<Op,descent_char::W<II...>> {
    using Beta = typename Op::template ApplyPack<typename Traverse<Op,II>::Beta...>;
  };
}

namespace descent_char {
  // ── 1. which components' rules() does BuildRules reach? ──────────────────────
  static_assert( BuildRules<Chain<>,Chain<API,A>>::rules(),
    "no rules anywhere: true");
  static_assert(!BuildRules<Chain<>,Chain<API,BadRule>>::rules(),
    "direct component: its rules() runs");
  static_assert(!BuildRules<Chain<>,Chain<API,Chain<BadRule>>>::rules(),
    "nested bare Chain is spliced (rules.h): its rules() runs");
  static_assert(!BuildRules<Chain<>,Chain<API,APIOf<API2,BadRule>>>::rules(),
    "nested APIOf is spliced (hapi.h): its rules() runs");
  static_assert( BuildRules<Chain<>,Chain<API,W<BadRule>>>::rules(),
    "[D2] a Traverse-visible wrapper is NOT spliced: the rules() inside it never run");

  // ...while queries DO see inside the same wrapper: what a rule can see and
  // what actually gets validated already disagree.
  static_assert( query<SameAs<BadRule>,Chain<W<BadRule>>>,
    "[D2] query sees through W via its Traverse specialization");
  static_assert( query<SameAs<BadRule>,Chain<Chain<BadRule>>>,
    "query sees through a nested Chain");
  static_assert(!query<SameAs<A>,Chain<W<BadRule>>>,
    "query does not invent matches");

  // ── 2. NoCollision reach ─────────────────────────────────────────────────────
  HAPI_DETECT_MEMBER(init);
  struct InitVoid {static void init() {}};
  struct InitInt  {static int  init() {return 1;}};   // collides with InitVoid
  static_assert(NoCollision<HapiMember_init,Chain<InitVoid,W<InitInt>>>,
    "[D2] a collision hidden inside a Traverse-visible wrapper is not detected");

  // ── 3. Traverse-based ops over an exact APIOf ─────────────────────────────────
  // (an APIOf, unlike Chain, has no Traverse specialization: it is a leaf)
  using CellA = APIOf<API,A>;
  static_assert( Exists<SameAs<CellA>,Chain<B,CellA>>::value,
    "[D3] Any matches an APIOf as a whole element");
  static_assert(!Exists<SameAs<A>,Chain<CellA>>::value,
    "[D3] Any does not look inside an APIOf");
  static_assert(!Exists<SameAs<API>,Chain<CellA>>::value,
    "[D3] ...nor at its API");
  static_assert(std::is_same<Eval<Filter<SameAs<CellA>>,Chain<B,CellA,B>>,Chain<CellA>>::value,
    "[D3] Filter selects the APIOf whole");
  static_assert(std::is_same<Eval<Filter<SameAs<A>>,Chain<CellA,A>>,Chain<A>>::value,
    "[D3] Filter does not descend into the APIOf (only the outer A matches)");
  template<typename T> struct Wrap {using Type=T*;};
  static_assert(std::is_same<Transform<Wrap,Chain<A,CellA>>,Chain<A*,CellA*>>::value,
    "[D3] Map transforms the APIOf whole, not its parts");

  // ── 4. FindFirst ─────────────────────────────────────────────────────────────
  static_assert(std::is_same<typename FindFirst<SameAs<A>>::template Check<Chain<B,A>>,A>::value,
    "finds in a flat Chain");
  static_assert(std::is_same<typename FindFirst<SameAs<A>>::template Check<Chain<B,Chain<C,A>>>,A>::value,
    "descends nested Chains");
  static_assert(!HasResult<FindFirst_<SameAs<A>,Chain<CellA>>>::value,
    "[D3] does not open an APIOf: A inside CellA is not found");
  static_assert(std::is_same<typename FindFirst<SameAs<CellA>>::template Check<Chain<B,CellA>>,CellA>::value,
    "matches an APIOf as a whole");

  // The pattern agnosticism's refid.h relies on: find the CELL (an APIOf) whose
  // ::Types carries a Tag. Q is tested on the whole cell, so any change that
  // opens containers before testing them would silently break this.
  using Cell3 = APIOf<API,Tag<3>>;
  using Cell7 = APIOf<API,Tag<7>,A>;
  static_assert(std::is_same<
      typename FindFirst<FromTypes<SameAs<Tag<7>>>>::template Check<Chain<Cell3,Cell7>>,Cell7>::value,
    "refid pattern: whole-cell match through FromTypes");
  // Whole-object selection (what OneMenu's View does with Filter<FromTypes<..>>).
  static_assert(std::is_same<
      Eval<Filter<FromTypes<SameAs<Tag<7>>>>,Chain<Cell3,Cell7>>,Chain<Cell7>>::value,
    "whole-object Filter through FromTypes");

  // Short-circuit: a hit must never instantiate a sibling after it. The
  // predicate static_asserts if it is ever applied to Poison.
  struct Poison {};
  struct Guard {
    template<typename O> struct Apply {
      static_assert(!std::is_same<O,Poison>::value,"FindFirst instantiated a sibling past the match");
      static constexpr bool value = std::is_same<O,A>::value;
    };
  };
  static_assert(std::is_same<typename FindFirst<Guard>::template Check<Chain<A,Poison>>,A>::value,
    "FindFirst stops at the first hit without touching later siblings");
}

int main() {return 0;}
