/**
 * @file expand_tests.cpp
 * @brief Expand<O> / Expansion / IsContainer (chain.h, hapi.h): what a container holds, compile-only.
 *
 * Expand is not used by any walk yet (descentUnify Phase 1); these pin the trait's own contract so
 * the later phases that route Traverse/FindFirst/BuildRules/NoCollision through it start from a
 * verified base. See tests/descent_characterization.cpp for how the walks behave today.
 */
#include "../include/hapi/hapi.h"
using namespace hapi;

namespace expand_test {
  struct API {};
  struct A {template<typename O> struct Part : O {using O::O;};};
  struct B {template<typename O> struct Part : O {using O::O;};};

  // ── leaves ───────────────────────────────────────────────────────────────────
  static_assert(!IsContainer<int>::value, "a plain type is a leaf");
  static_assert(!IsContainer<A>::value,   "a component is a leaf");

  // ── Chain: transparent to every walk ──────────────────────────────────────────
  static_assert(IsContainer<Chain<A,B>>::value, "Chain is a container");
  static_assert(IsContainer<Chain<>>::value,    "an empty Chain is still a container");
  static_assert(std::is_same<Expand<Chain<A,B>>::Children,Chain<A,B>>::value, "Chain expands to itself");
  static_assert(Expand<Chain<A>>::queried && Expand<Chain<A>>::selected &&
                Expand<Chain<A>>::validates && Expand<Chain<A>>::searched,
    "every walk descends a Chain");

  // ── APIOf: API first, then components (D1) ────────────────────────────────────
  using Cell = APIOf<API,A,B>;
  static_assert(IsContainer<Cell>::value, "APIOf is a container");
  static_assert(std::is_same<Expand<Cell>::Children,Chain<API,A,B>>::value, "API first, then the components");
  static_assert(std::is_same<Expand<Cell>::Children,Cell::Types>::value,
    "Expand<APIOf> is exactly APIOf::Types, the list rules validate over");
  static_assert(!Expand<Cell>::queried && !Expand<Cell>::selected && !Expand<Cell>::searched,
    "today an APIOf is a leaf for queries, Filter/Map and FindFirst: those stay off");
  static_assert( Expand<Cell>::validates,
    "today BuildRules/NoCollision splice a nested APIOf: that stays on");

  // ── opt-in is per exact type, never inferred ──────────────────────────────────
  // A type carrying ::Types is NOT thereby a container (hapi.h, commit 7c5e779).
  struct HasTypesOnly {using Types = Chain<A,B>;};
  static_assert(!IsContainer<HasTypesOnly>::value, "::Types alone does not make a container");

  // A type deriving from APIOf is a different type: it needs its own entry.
  template<typename... OO> struct Plain : APIOf<API,OO...> {};   // never given an Expand entry
  static_assert(!IsContainer<Plain<A>>::value, "a derived type is not a container until it says so");

  // Note: an Expand specialization must be declared BEFORE the first use of Expand/IsContainer on that type
  // (g++ rejects "partial specialization after instantiation"; clang lets it slide), same rule as Traverse today.
  // Hence Item below is kept out of every assert until its entry has been declared.
  template<typename... OO> struct Item : APIOf<API,OO...> {};
}
namespace hapi {
  // the one-line forward for a derived type: same children, same policy as its base
  template<typename... OO>
  struct Expand<expand_test::Item<OO...>> : Expand<APIOf<expand_test::API,OO...>> {};
}
namespace expand_test {
  static_assert(IsContainer<Item<A,B>>::value, "forwarded: now a container");
  static_assert(std::is_same<Expand<Item<A,B>>::Children,Chain<API,A,B>>::value, "forwarded: same children as its base");

  // an entry can enable exactly the bits it wants (e.g. a wrapper queries see into, but Filter takes whole)
  template<typename... II> struct Box {template<typename O> struct Part : O {using O::O;};};
}
namespace hapi {
  template<typename... II>
  struct Expand<expand_test::Box<II...>> : Expansion<Chain<II...>,true> {};
}
namespace expand_test {
  static_assert( Expand<Box<A>>::queried,  "Expansion<..,true>: queried on");
  static_assert(!Expand<Box<A>>::selected && !Expand<Box<A>>::validates && !Expand<Box<A>>::searched,
    "every other bit defaults to off");
}

int main() {return 0;}
