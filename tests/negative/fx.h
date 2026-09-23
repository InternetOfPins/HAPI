// Shared fixtures for tests/negative/*.cpp. A and B mirror tests/compile_tests.cpp (B's rules() verbatim),
// so the commented-out fail_* cases there are re-enabled here with the same meaning.
#pragma once
#include "../../include/hapi/hapi.h"
using namespace hapi;

struct API {};  struct API2 {};
struct A {template<typename O> struct Part : O {using O::O;};};
struct B {
  template<typename O> struct Part : O {using O::O;};
  template<typename Before,typename After> static constexpr bool rules() {
    static_assert(query<SameAs<A>,Before>,"B only makes sense after A somehow :D");
    static_assert(!query<SameAs<B>,After>,"do not repeat B!");
    static_assert(!query<SameAs<A>,After>,"A must be before B");
    return true;
  }
};
// a component whose rules() simply fails, to test that a walk reaches it
struct BadRule {
  template<typename Bf,typename Af> static constexpr bool rules() {return false;}
  template<typename O> struct Part : O {using O::O;};
};
template<typename... OO> struct ItemDef : APIOf<API,OO...> {
  using Base=APIOf<API,OO...>;
  using Base::Base;
};

// NoCollision fixtures (mirror compile_tests.cpp): same member name, different signatures
HAPI_DETECT_MEMBER(init);
struct WithInitVoid {template<typename O> struct Part : O {using Base=O; using Base::Base; static void init() {}};};
struct TerminalInitInt {static int init() {return 1;}};
