/**
 * @file compile_tests.cpp
 * @brief HAPI Compile-time validation tests
 * 
 * This file is meant to be compiled (not necessarily run).
 * Heavy use of static_assert + type traits.
 */

#include "../include/hapi/hapi.h"
using namespace hapi;

#ifdef __AVR__
  #include <streamFlow.h>
  using namespace StreamFlow;
  #define cout Serial
#else
  #include <iostream>
  using namespace std;
#endif

template<typename Cfg=Nil>
struct ItemAPI:Cfg{
  template<typename Out>
  static constexpr void print(Out& out) {out<<"/";}
};

template<typename... OO>
struct ItemDef:APIOf<ItemAPI<>,OO...>{
  using Base=APIOf<ItemAPI<>,OO...>;
  using Base::Base;
  static constexpr const size_t size{sizeof...(OO)};
};

//stream output for items --
template<typename Out,typename... OO>
Out& operator<<(Out& out,const ItemDef<OO...>& o) {o.print(out);return out;}

//rules ItemDef query specialization --
template<typename Q,typename... OO>
constexpr const bool query<Q,ItemDef<OO...>>{(query<Q,OO>||...)};

struct A {
  template<typename O>
  struct Part:O {
    using Base=O;
    using Base::Base;
    template<typename Out> 
    static constexpr void print(Out& out) {
      out<<"/A";
      Base::print(out);
    }
  };
};

struct B {
  template<typename O>
  struct Part:O {
    using Base=O;
    using Base::Base;
    template<typename Out>
    static constexpr void print(Out& out) {
      out<<"/B";
      Base::print(out);
    }
  };
  template<typename Before,typename After>
  static constexpr bool rules() {
    static_assert(query<SameAs<A>,Before>,"B only makes sense after A somehow :D");
    static_assert(!query<SameAs<B>,After>,"do not repeat B!");
    static_assert(!query<SameAs<A>,After>,"A must be before B");
    return true;
  }
};

constexpr ItemDef<A,B> ok{};
constexpr ItemDef<A,A,B> ok2{};
constexpr ItemDef<Chain<A,B>> ok3{};//A,B nested one level (mono_block) -- same rules() must still fire
// constexpr ItemDef<B> fail_requireA{};//will fail with compile error
// constexpr ItemDef<B,A> fail_order{};//will fail with compile error "error: static assertion failed: A must be before B"
// constexpr ItemDef<A,B,B> fail_unicity{};//will fail with compile error "error: static assertion failed: do not repeat B!""
// constexpr ItemDef<Chain<B,A>> fail_nested_order{};//same as fail_order, nested: "error: static assertion failed: A must be before B"

// ── meta.h surface coverage ────────────────────────────────────────────────
// Everything below exercises a meta.h (or rules.h) construct that had zero
// compile coverage before -- the At<idx,O> termination bug (fixed 2026-08-20,
// see the At<0,AtNoBase> case below) went unnoticed for exactly this reason:
// nothing here ever instantiated it, so only a downstream consumer could hit
// it. All static_assert, not run() prints -- these are type-level checks.

struct C {};

// -- Not / And / Or --
static_assert( Not<SameAs<A>>::template Apply<B>::value,  "Not: B is not A");
static_assert(!Not<SameAs<A>>::template Apply<A>::value,  "Not: A is A");
static_assert( And<SameAs<A>,SameAs<A>>::template Apply<A>::value,  "And: both match");
static_assert(!And<SameAs<A>,SameAs<B>>::template Apply<A>::value,  "And: one mismatches");
static_assert( Or<SameAs<A>,SameAs<B>>::template Apply<B>::value,   "Or: second matches");
static_assert(!Or<SameAs<A>,SameAs<C>>::template Apply<B>::value,   "Or: neither matches");

// -- TagIs / IsInstanceOf --
struct MyTag {};
struct Tagged : MyTag {};
static_assert( TagIs<MyTag>::template Apply<Tagged>::value, "TagIs: Tagged derives MyTag");
static_assert(!TagIs<MyTag>::template Apply<A>::value,      "TagIs: A does not derive MyTag");

template<typename...> struct SomeTemplate {};
static_assert( IsInstanceOf<SomeTemplate>::template Apply<SomeTemplate<int>>::value, "IsInstanceOf: matches");
static_assert(!IsInstanceOf<SomeTemplate>::template Apply<A>::value,                 "IsInstanceOf: A is not a SomeTemplate<...>");

// -- Any (fold) --
static_assert( Any<SameAs<A>>::template Check<Chain<B,A>>::value,  "Any: A is present");
static_assert(!Any<SameAs<A>>::template Check<Chain<B,B>>::value,  "Any: A is absent");

// -- Partition --
static_assert(std::is_same_v<Eval<Partition<SameAs<A>>,A>, Left<A>>,  "Partition: match goes Left");
static_assert(std::is_same_v<Eval<Partition<SameAs<A>>,B>, Right<B>>, "Partition: mismatch goes Right");

// -- Map / Transform --
template<typename T> struct AddPtr { using Type = T*; };
static_assert(std::is_same_v<Transform<AddPtr,Chain<A,B>>, Chain<A*,B*>>, "Transform: maps every element");

// -- Filter --
static_assert(std::is_same_v<Eval<Filter<SameAs<A>>,Chain<A,B,A>>, Chain<A,A>>, "Filter: keeps only matches, in order");
static_assert(std::is_same_v<Eval<Filter<SameAs<C>>,Chain<A,B>>,   Chain<>>,    "Filter: no matches yields an empty Chain");

// -- FindFirst / FindFirstOr --
static_assert(std::is_same_v<typename FindFirst<SameAs<B>>::template Check<Chain<A,B>>, B>, "FindFirst: locates the match");
static_assert(std::is_same_v<typename FindFirstOr<SameAs<A>,Nil>::template Check<Chain<B,A>>, A>,   "FindFirstOr: hit returns the match");
static_assert(std::is_same_v<typename FindFirstOr<SameAs<A>,Nil>::template Check<Chain<B,B>>, Nil>, "FindFirstOr: miss falls back to Default");

// -- FromTypes (drill into ::Types, only true if the member exists) --
static_assert( FromTypes<SameAs<A>>::template Apply<decltype(ok)>::value, "FromTypes: A is reachable via ok::Types");
static_assert(!FromTypes<SameAs<C>>::template Apply<decltype(ok)>::value, "FromTypes: C is not in ok::Types");
static_assert(!FromTypes<SameAs<A>>::template Apply<int>::value,          "FromTypes: int has no ::Types member at all");

// -- Requires / Excludes (rules.h; the real replacement for the dead
// require<>/exclude<> convenience wrappers this file and examples/rules
// used to duplicate locally -- both deleted, this is their only coverage) --
static_assert( Requires<SameAs<A>,Chain<A,B>>, "Requires: A is present");
static_assert(!Requires<SameAs<C>,Chain<A,B>>, "Requires: C is absent");
static_assert( Excludes<SameAs<C>,Chain<A,B>>, "Excludes: C is absent");
static_assert(!Excludes<SameAs<A>,Chain<A,B>>, "Excludes: A is present");

// -- At<idx,O> / at<idx>(obj) --
// Real inheritance (not just a ::Base alias) so at<>()'s static_cast upcast
// is well-formed -- mirrors how a real Chain::Part<T> composition looks.
struct AtL2 {};
struct AtL1 : AtL2 { using Base = AtL2; };
struct AtL0 : AtL1 { using Base = AtL1; };
static_assert(std::is_same_v<At<0,AtL0>::Type, AtL0>, "At<0,O>: terminal case is O itself");
static_assert(std::is_same_v<At<1,AtL0>::Type, AtL1>, "At<1,O>: one level up via O::Base");
static_assert(std::is_same_v<At<2,AtL0>::Type, AtL2>, "At<2,O>: two levels up via O::Base::Base");

// Regression guard for the fixed conditional_t bug (2026-08-20, commit 8b404d0):
// idx==0 must NOT require O::Base to exist -- the old
// conditional_t<(idx>0),...,O> eagerly instantiated both branches, so even
// this terminal case needed a ::Base member. AtNoBase has none, matching
// hapi::Nil (the real Chain<> terminal) -- this is exactly the shape that used
// to fail to compile.
struct AtNoBase {};
static_assert(std::is_same_v<At<0,AtNoBase>::Type, AtNoBase>, "At<0,O>: must not require O::Base to exist");

// const propagation: At<idx,O> threads IsConst separately from the O::Base
// walk, since a nested typedef name never carries constness on its own --
// without that, at<1>(constObj) would silently compute a non-const Type and
// then fail to compile ("casting away const") one level up.
static_assert(std::is_same_v<At<0,const AtL0>::Type, const AtL0>, "At<0,const O>: stays const");
static_assert(std::is_same_v<At<1,const AtL0>::Type, const AtL1>, "At<1,const O>: const survives the walk");

// at<idx>(obj): three overloads (&, *, &&) instead of the old auto-NTTP form
// (template<auto ref>), which needed a class-type object bound BY VALUE as a
// non-type template parameter -- C++20-only (P1907), so it could never be
// called under this project's C++17 ceiling. Found while first writing this
// coverage; fixed by dropping the NTTP entirely in favor of a normal
// function parameter, which also lifts the old "must have static storage
// duration" restriction a linkage-requiring NTTP could never avoid either.
inline AtL0 atLvalue{};
static_assert(std::is_same_v<decltype(at<1>(atLvalue)), AtL1&>, "at<idx>(T&): resolves through the & overload");
static_assert(std::is_same_v<decltype(at<1>(&atLvalue)), AtL1&>, "at<idx>(T*): resolves through the * overload");
static_assert(std::is_same_v<decltype(at<1>(AtL0{})), AtL1&&>, "at<idx>(T&&): resolves through the && overload");

void run() {
  cout<<"HasRules<A>:"<<HasRules<A>::value<<endl;
  cout<<"HasRules<B>:"<<HasRules<B>::value<<endl;
  cout<<"query<SameAs<A>,A>:"<<query<SameAs<A>,A><<endl;
  cout<<"query<SameAs<A>,Chain<A>>:"<<query<SameAs<A>,Chain<A>><<endl;
  cout<<endl;

  // find(): runtime, takes C& (non-const) -- ok/ok2/ok3 above are constexpr
  // (implicitly const), so this needs its own mutable instance.
  ItemDef<A,B> okMutable{};
  find<SameAs<A>>(okMutable); // compiles only if A is reachable via ::Types; nothing to print, presence is the test

  // at<idx>(obj): confirm all three overloads actually alias the same object
  // at runtime, not just at the type level. The && case binds to a named
  // reference first -- calling at<1>(AtL0{}) directly would return a
  // reference into a temporary destroyed at the end of that expression.
  cout<<"at<1>(atLvalue) aliases atLvalue:"<<(&at<1>(atLvalue)==(void*)&atLvalue)<<endl;
  cout<<"at<1>(&atLvalue) aliases atLvalue:"<<(&at<1>(&atLvalue)==(void*)&atLvalue)<<endl;
  AtL0&& atTemp = AtL0{};
  cout<<"at<1>(named tmp) aliases tmp:"<<(&at<1>(atTemp)==(void*)&atTemp)<<endl;
  cout<<endl;
}

#ifdef ARDUINO
  void setup() {
    Serial.begin(115200);
    while(!Serial);
  }
  void loop() {
    run();
    delay(1000);
  }
#else
  int main() {
    cout<<std::boolalpha;
    run();
    return 0;
  }
#endif
 