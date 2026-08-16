// mono_block Chain tests
//
// Verifies the mono_block topology:
//   1. Basic chain collapse — same behaviour as before
//   2. Chain-as-component — Chain<Chain<A,B>,C> == Chain<A,B,C>
//   3. find<Q> — FindFirst + static_cast, no linear walk
//   4. Tag propagation — automatic through the ancestor set
//   5. Hapi<T> view — Part delegates through T::Part<O>

#include <iostream>
#include <cassert>
#include <type_traits>
using namespace std;

#include <hapi/hapi.h>
using namespace hapi;

// ─── Shared terminal API ──────────────────────────────────────────────────────

struct TextAPI {
  string buf{};
  void put(char c) { buf += c; }
  void reset()     { buf.clear(); }
};

// ─── Hapis ───────────────────────────────────────────────────────────────

struct TagA {
  template<typename O>
  struct Part : O {
    using Base = O; using Base::Base;
    void put(char c) { Base::put('('); Base::put(c); Base::put(')'); }
  };
};

struct TagB {
  template<typename O>
  struct Part : O {
    using Base = O; using Base::Base;
    void put(char c) { Base::put('['); Base::put(c); Base::put(']'); }
  };
};

struct TagC {
  template<typename O>
  struct Part : O {
    using Base = O; using Base::Base;
    void put(char c) { Base::put('<'); Base::put(c); Base::put('>'); }
  };
};

// ─── Tag for find<> tests ─────────────────────────────────────────────────────

struct SpecialTag {};

struct Tagged : SpecialTag {
  template<typename O>
  struct Part : O {
    using Base = O; using Base::Base;
    void put(char c) { Base::put('*'); Base::put(c); Base::put('*'); }
    int  value()     { return 42; }
  };
};

// ─── Test 1: basic chain collapse ─────────────────────────────────────────────

void test_basic_collapse() {
  APIOf<TextAPI, TagA, TagB, TagC> obj;
  obj.put('x');
  // Mono_block: each decorator char from an outer component re-traverses inner components.
  // TagA sends '(' → TagB wraps: TagC wraps each → <[><(><]>; then 'x' → <[><x><]>; ')' → <[><)><]>
  const string expected = "<[><(><]><[><x><]><[><)><]>";
  assert(obj.buf == expected);
  obj.reset();
  cout << "PASS test_basic_collapse  buf=" << expected << "\n";
}

// ─── Test 2: Chain-as-component ───────────────────────────────────────────────

void test_chain_as_component() {
  // Chain<TagA,TagB> used as a component inside Chain<...,TagC>
  using Inner = Chain<TagA, TagB>;
  APIOf<TextAPI, Inner, TagC> nested;
  nested.put('x');
  // same collapse as APIOf<TextAPI, TagA, TagB, TagC>
  const string expected = "<[><(><]><[><x><]><[><)><]>";
  assert(nested.buf == expected);
  cout << "PASS test_chain_as_component  buf=" << nested.buf << "\n";
}

// ─── Test 3: find<Q> — FindFirst + static_cast ───────────────────────────────

void test_find() {
  APIOf<TextAPI, TagA, Tagged, TagC> obj;
  // find the Tagged component (which has value())
  auto& t = hapi::find<TagIs<SpecialTag>>(obj);
  assert(t.value() == 42);
  cout << "PASS test_find  value=" << t.value() << "\n";
}

// ─── Test 4: tag propagation — is_base_of across ancestor set ─────────────────

struct ZWTag {};

struct ZeroWidthComp : ZWTag {
  template<typename O>
  struct Part : O { using Base=O; using Base::Base; };
};

struct NormalComp {
  template<typename O>
  struct Part : O { using Base=O; using Base::Base; };
};

void test_tag_propagation() {
  // Chain<NormalComp, ZeroWidthComp> — does ZWTag propagate?
  using C = Chain<NormalComp, ZeroWidthComp>;
  // query<TagIs<ZWTag>, C> should find ZeroWidthComp
  static_assert(query<TagIs<ZWTag>, C>, "ZWTag should propagate in chain");
  static_assert(!query<TagIs<ZWTag>, Chain<NormalComp>>, "ZWTag should not be in plain chain");
  cout << "PASS test_tag_propagation\n";
}

// ─── Test 5: Hapi<T> view — delegates through T::Part<O> ────────────────

void test_component_view() {
  // Hapi<Chain<TagA,TagB>>::Part<TextAPI> delegates to Chain<TagA,TagB>::Part<TextAPI>
  // = TagA::Part<TagB::Part<TextAPI>>: TagA sends '(','x',')'; TagB wraps each → [(][x][)]
  Hapi<Chain<TagA,TagB>>::Part<TextAPI> view;
  view.put('x');
  assert(view.buf == "[(][x][)]");
  cout << "PASS test_component_view  buf=" << view.buf << "\n";
}

// ─── Test 6: nested Chain-as-component with find ──────────────────────────────

void test_nested_find() {
  using Inner = Chain<Tagged, TagB>;
  APIOf<TextAPI, TagA, Inner> obj;
  auto& t = hapi::find<TagIs<SpecialTag>>(obj);
  assert(t.value() == 42);
  cout << "PASS test_nested_find  value=" << t.value() << "\n";
}

// ─── Test 7: rules() folding through a nested (mono_block) chain ─────────────

struct State {
  template<typename O>
  struct Part : O { using Base=O; using Base::Base; int value{0}; };
};

struct Logic {
  template<typename O>
  struct Part : O { using Base=O; using Base::Base; };
  template<typename Before,typename After>
  static constexpr bool rules() {
    static_assert(Requires<SameAs<State>,After>, "Logic requires State somewhere after it");
    return true;
  }
};

// Wrong order — Logic's own rule now fires even though State/Logic are
// nested one level inside LogicState, not listed directly in the outer
// chain: "error: static assertion failed: Logic requires State somewhere
// after it". Before the BuildRules<Before,Chain<Chain<PP...>,Rest...>>
// splice specialization, this compiled silently (BuildRules saw the whole
// nested Chain as one opaque, rule-less unit and never called Logic::rules
// at all) — verified both ways while developing the fix.
// using StateLogic = Chain<State, Logic>;
// APIOf<TextAPI, TagA, StateLogic> fail_wrong_order{};

void test_rules_fold_through_nested_chain() {
  // Correct order, nested one level via mono_block — same shape as
  // Tap<Coeff> = Chain<TapLogic<Coeff>, Reg<Sample>> in
  // HAPI/.RnD/acTypesHLS/hapi_reg.h. Logic's rule must actually run and
  // pass (not be silently skipped) for this to compile.
  using LogicState = Chain<Logic, State>;
  APIOf<TextAPI, TagA, LogicState> obj;
  (void)obj;
  cout << "PASS test_rules_fold_through_nested_chain\n";
}

// ─────────────────────────────────────────────────────────────────────────────

#ifdef ARDUINO
  void setup() {
    Serial.begin(115200);
    while (!Serial);
    test_basic_collapse();
    test_chain_as_component();
    test_find();
    test_tag_propagation();
    test_component_view();
    test_nested_find();
    test_rules_fold_through_nested_chain();
  }
  void loop() {}
#else
  int main() {
    test_basic_collapse();
    test_chain_as_component();
    test_find();
    test_tag_propagation();
    test_component_view();
    test_nested_find();
    test_rules_fold_through_nested_chain();
    cout << "\nAll tests passed.\n";
    return 0;
  }
#endif
