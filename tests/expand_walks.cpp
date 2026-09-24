/**
 * @file expand_walks.cpp
 * @brief Each Expand policy bit opens exactly one family of walks (compile-only).
 *
 * One test container per bit (Q=queried, S=selected, V=validates, F=searched), each enabling ONLY its bit.
 * For each, the walk it is meant for opens it, and every other walk treats it as a whole element. This
 * is the mechanism the later policy flips rely on (e.g. an ItemDef with selected off but queried on).
 * The rule-violating variants (a failing rules()/collision inside V) are in tests/negative/.
 */
#include "../include/hapi/hapi.h"
using namespace hapi;

namespace walk_test {
  struct API {};
  struct A {template<typename O> struct Part : O {using O::O;};};
  struct B {template<typename O> struct Part : O {using O::O;};};
  struct BadRule {
    template<typename Bf,typename Af> static constexpr bool rules() {return false;}
    template<typename O> struct Part : O {using O::O;};
  };
  HAPI_DETECT_MEMBER(init);
  struct InitVoid {static void init() {}};
  struct InitInt  {static int  init() {return 1;}};
  struct Poison {};

  template<typename... II> struct Q {template<typename O> struct Part : O {using O::O;};};   // queried only
  template<typename... II> struct S {template<typename O> struct Part : O {using O::O;};};   // selected only
  template<typename... II> struct V {template<typename O> struct Part : O {using O::O;};};   // validates only
  template<typename... II> struct F {template<typename O> struct Part : O {using O::O;};};   // searched only
  // validates, AND carries a rule of its own: no A anywhere after it (what ItemPrinter's "Cursor below" rule is like)
  template<typename... II> struct VO {
    template<typename Bf,typename Af> static constexpr bool rules() {return !Requires<SameAs<A>,Af>;}
    template<typename O> struct Part : O {using O::O;};
  };
}
namespace hapi {
  template<typename... II> struct Expand<walk_test::Q<II...>> : Expansion<Chain<II...>,true, false,false,false> {};
  template<typename... II> struct Expand<walk_test::S<II...>> : Expansion<Chain<II...>,false,true, false,false> {};
  template<typename... II> struct Expand<walk_test::V<II...>> : Expansion<Chain<II...>,false,false,true, false> {};
  template<typename... II> struct Expand<walk_test::F<II...>> : Expansion<Chain<II...>,false,false,false,true > {};
  template<typename... II> struct Expand<walk_test::VO<II...>> : Expansion<Chain<II...>,false,false,true, false> {};
}

namespace walk_test {
  template<typename T> struct Wrap {using Type=T*;};
  template<typename Q_,typename L> using Found = HasResult<FindFirst_<Q_,L>>;

  // ── queried: Any/Exists look inside; selection, rules and FindFirst take Q whole ──
  static_assert( Exists<SameAs<A>,Chain<Q<A>>>::value,                              "queried: Any opens it");
  static_assert(!Exists<SameAs<A>,Chain<S<A>>>::value &&
                !Exists<SameAs<A>,Chain<V<A>>>::value &&
                !Exists<SameAs<A>,Chain<F<A>>>::value,                              "the other bits do not open it for Any");
  static_assert(std::is_same<Eval<Filter<SameAs<A>>,Chain<Q<A>,A>>,Chain<A>>::value,"queried: Filter does not open it (only the outer A)");
  static_assert(std::is_same<Transform<Wrap,Chain<Q<A>>>,Chain<Q<A>*>>::value,       "queried: Map takes it whole");
  static_assert( BuildRules<Chain<>,Chain<API,Q<BadRule>>>::rules(),                 "queried: rules inside are not run");
  static_assert(!Found<SameAs<A>,Chain<Q<A>>>::value,                                "queried: FindFirst does not open it");

  // ── selected: Filter/Map look inside; Any, rules and FindFirst do not ─────────────
  static_assert(std::is_same<Eval<Filter<SameAs<A>>,Chain<S<A>,A>>,Chain<A,A>>::value,"selected: Filter opens it (the A inside, then the outer A)");
  static_assert(std::is_same<Transform<Wrap,Chain<S<A>>>,Chain<Chain<A*>>>::value,    "selected: Map opens it and rebuilds the shape");
  static_assert( BuildRules<Chain<>,Chain<API,S<BadRule>>>::rules(),                  "selected: rules inside are not run");
  static_assert(!Found<SameAs<A>,Chain<S<A>>>::value,                                 "selected: FindFirst does not open it");

  // ── validates: BuildRules/NoCollision splice it; queries, selection, FindFirst do not ──
  static_assert(!BuildRules<Chain<>,Chain<API,V<BadRule>>>::rules(),                  "validates: the rules() inside run");
  static_assert( BuildRules<Chain<>,Chain<API,V<A>>>::rules(),                        "validates: and pass when they should");
  static_assert(!Exists<SameAs<BadRule>,Chain<V<BadRule>>>::value,                    "validates: Any does not open it");
  static_assert(std::is_same<Eval<Filter<SameAs<A>>,Chain<V<A>>>,Chain<>>::value,     "validates: Filter does not open it");
  static_assert(!Found<SameAs<A>,Chain<V<A>>>::value,                                 "validates: FindFirst does not open it");
  static_assert(NoCollision<HapiMember_init,Chain<InitVoid,S<InitInt>>>,
    "a colliding member inside a non-validating container is not seen (a validating one is: tests/negative)");

  // ── validates + a rules() of its own: splicing its children must not drop the container's own rule ──
  static_assert( BuildRules<Chain<>,Chain<API,VO<>,B>>::rules(),                      "own rule runs, and passes when it should");
  static_assert(!BuildRules<Chain<>,Chain<API,VO<>,A>>::rules(),                      "own rule runs: it sees its later siblings (as if placed directly)");
  static_assert( BuildRules<Chain<>,Chain<API,VO<A>>>::rules(),                       "...but not its own children (same view as a directly placed component)");
  static_assert(!BuildRules<Chain<>,Chain<API,VO<BadRule>>>::rules(),                 "and its children's rules run too");

  // ── searched: FindFirst opens it, node first; everything else does not ─────────────
  static_assert(std::is_same<typename FindFirst<SameAs<A>>::template Check<Chain<F<A>>>,A>::value,
    "searched: FindFirst opens it");
  static_assert(std::is_same<typename FindFirst<SameAs<F<A>>>::template Check<Chain<F<A>>>,F<A>>::value,
    "searched: the container itself can be the match");
  static_assert(std::is_same<typename FindFirst<Or<SameAs<F<A>>,SameAs<A>>>::template Check<Chain<F<A>>>,F<A>>::value,
    "searched: the node is tested BEFORE its children (refid relies on this)");
  static_assert(!Exists<SameAs<A>,Chain<F<A>>>::value &&
                std::is_same<Eval<Filter<SameAs<A>>,Chain<F<A>>>,Chain<>>::value &&
                BuildRules<Chain<>,Chain<API,F<BadRule>>>::rules(),
    "searched: Any, Filter and rules do not open it");

  // short-circuit survives opening a container: nothing after the hit is instantiated
  struct Guard {
    template<typename O> struct Apply {
      static_assert(!std::is_same<O,Poison>::value,"FindFirst instantiated a sibling past the match");
      static constexpr bool value = std::is_same<O,A>::value;
    };
  };
  static_assert(std::is_same<typename FindFirst<Guard>::template Check<Chain<F<A,Poison>,Poison>>,A>::value,
    "the hit inside an opened container stops the search: neither its later children nor later siblings are touched");
  static_assert(std::is_same<typename FindFirst<Guard>::template Check<Chain<F<B,F<A>>,Poison>>,A>::value,
    "and containers nest");
}

int main() {return 0;}
