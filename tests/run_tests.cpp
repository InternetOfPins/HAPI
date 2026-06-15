/**
 * @file run_tests.cpp
 * @brief Runtime tests for hapi/run.h
 *
 * Covers:
 *   - Runtime predicates: Equal, Less, Greater (data member, not template param)
 *   - AND fold chain with True terminal
 *   - OR fold chain with False terminal
 *   - Not expression
 *   - runEach: small chain (<=8, forEach path) and large chain (>8, table loop path)
 */

#include "../include/hapi/hapi.h"
#include "../include/hapi/run.h"
using namespace hapi::run;

#ifdef __AVR__
  #include <streamFlow.h>
  using namespace StreamFlow;
  #define cout Serial
  #define assert(x) do { if(!(x)) { Serial.print("FAIL: "); Serial.println(#x); } } while(0)
#else
  #include <iostream>
  #include <cassert>
  using namespace std;
#endif

// ====================== Helpers ======================

// Checkable tags the component (outer struct) — no diamond in chain.
template<size_t I>
struct Comp : Checkable {
  template<typename O>
  struct Part : O {
    int val{(int)I};
  };
};

// ====================== Tests ======================

void test_equal() {
  using Node = hapi::APIOf<True, Equal<int>>;
  Node n;
  n.q = 5;
  assert( n.check(5));
  assert(!n.check(4));
  cout << "equal: ok" << endl;
}

void test_less() {
  using Node = hapi::APIOf<True, Less<int>>;
  Node n;
  n.q = 10;
  assert( n.check(9));
  assert(!n.check(10));
  cout << "less: ok" << endl;
}

void test_greater() {
  using Node = hapi::APIOf<True, Greater<int>>;
  Node n;
  n.q = 3;
  assert( n.check(4));
  assert(!n.check(3));
  cout << "greater: ok" << endl;
}

void test_and_fold() {
  // range: v > 2 && v < 10
  using Node = hapi::APIOf<True, Greater<int>, Less<int>>;
  Node range;
  hapi::find<hapi::SameAs<Greater<int>>>(range).q = 2;
  hapi::find<hapi::SameAs<Less<int>>>(range).q = 10;
  assert( range.check(5));
  assert( range.check(3));
  assert( range.check(9));
  assert(!range.check(2));
  assert(!range.check(10));
  cout << "and_fold (range): ok" << endl;
}

void test_or_fold() {
  // equals 1 OR equals 7
  using A = EqualOr<int>;
  using B = EqualOr<int>;
  // Two different component wrappers needed for find<> disambiguation.
  // Use direct chain access: first node is topmost.
  using Node = hapi::APIOf<False, A, B>;
  Node oneof;
  // A is first in chain — accessible as top-level member
  oneof.q = 1;
  // B is the second — chain suffix: Chain<B>::Part<False>
  static_cast<hapi::Chain<B>::Part<False>&>(oneof).q = 7;
  assert( oneof.check(1));
  assert( oneof.check(7));
  assert(!oneof.check(0));
  cout << "or_fold (one-of): ok" << endl;
}

void test_not() {
  using Node = hapi::APIOf<True, Not<Equal<int>>>;
  Node n;
  // Not<Equal<int>>::Part inherits from Equal<int>::Part — q is accessible
  n.q = 5;
  assert(!n.check(5));
  assert( n.check(4));
  assert( n.check(6));
  cout << "not: ok" << endl;
}

void test_run_each_small() {
  // N=5, <=8 => forEach path (inlined and eliminated for empty lambda)
  using Node = hapi::APIOf<hapi::Nil, Comp<0>, Comp<1>, Comp<2>, Comp<3>, Comp<4>>;
  Node node;
  volatile int count = 0;
  runEach<hapi::TagIs<Checkable>>(node, [&count](auto&) { count++; });
  assert(count == 5);
  cout << "runEach small (forEach path, N=5): ok" << endl;
}

void test_run_each_large() {
  // N=12, >8 => table loop path
  using Node = hapi::APIOf<hapi::Nil,
    Comp<0>, Comp<1>, Comp<2>, Comp<3>, Comp<4>,
    Comp<5>, Comp<6>, Comp<7>, Comp<8>, Comp<9>,
    Comp<10>, Comp<11>>;
  Node node;
  volatile int count = 0;
  runEach<hapi::TagIs<Checkable>>(node, [&count](auto&) { count++; });
  assert(count == 12);
  cout << "runEach large (table loop, N=12): ok" << endl;
}

void test_run_each_sum() {
  // Verify val values are accessible through the lambda
  using Node = hapi::APIOf<hapi::Nil, Comp<0>, Comp<1>, Comp<2>, Comp<3>, Comp<4>>;
  Node node;
  volatile int sum = 0;
  runEach<hapi::TagIs<Checkable>>(node, [&sum](auto& part) { sum += part.val; });
  assert(sum == 0 + 1 + 2 + 3 + 4);
  cout << "runEach sum (N=5): ok" << endl;
}

// ====================== Entry ======================

void doTests() {
  test_equal();
  test_less();
  test_greater();
  test_and_fold();
  test_or_fold();
  test_not();
  test_run_each_small();
  test_run_each_large();
  test_run_each_sum();
  cout << "all run tests passed" << endl;
}

#ifdef ARDUINO
  void setup() { Serial.begin(115200); while(!Serial); }
  void loop() { doTests(); delay(5000); }
#else
  int main() {
    cout << boolalpha;
    doTests();
    return 0;
  }
#endif
