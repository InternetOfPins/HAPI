/**
 * @file strict_broker.cpp
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief Design-by-Contract via HAPI's Chain<>/Part<> axis: a broker
 *        named per component, missing checks are a hard compile error.
 *
 * A `contract::` overload set (plain free functions, one per checked
 * component's Part<O>) gates entry into a real chain call. `StrictBroker`
 * names exactly the methods it wants checked and calls into `contract::`
 * for those; anything it doesn't name passes straight through via
 * ordinary inheritance, zero cost. A missing `contract::` overload for a
 * named method is a hard compile error -- see hard_error_demo.cpp.
 *
 * `check()` is the only domain-specific piece here (aborts, fine for a
 * host demo; a real build would swap in something per-domain). It must
 * never return normally on a failed condition -- the "don't start the
 * call" guarantee is entirely check()'s contract, not code ordering.
 */

#include <hapi/hapi.h>
#include <cstdio>
#include <cstdlib>

using namespace hapi;

inline void check(bool cond, const char* msg) {
  if (!cond) { std::printf("CONTRACT VIOLATION: %s\n", msg); std::abort(); }
}

// Real terminal, with a bit of live state so a check can show it may
// reach past its own call's argument into whatever else the component
// it's checking exposes.
struct RawApi {
  int budget = 8;
  int f(int x) { budget -= x; return x * 2; }
  int remaining() const { return budget; }
};

// Ordinary, unrelated components. Neither knows the other exists, neither
// knows contract:: exists -- nothing here changes to get checked.
struct Foo {
  template<typename O>
  struct Part : O {
    using Base = O;
    int f(int x) { return Base::f(x) + 1; }
  };
};

struct Baz {
  template<typename O>
  struct Part : O {
    using Base = O;
    int f(int x) { return Base::f(x) * 10; }
  };
};

namespace contract {
  template<typename O>
  auto f(typename Foo::template Part<O>& self, int x) {
    check(x != 0, "Foo::f: x must not be 0");
    return self.f(x);
    // no postcondition -- optional per component, this one has none
  }

  template<typename O>
  auto f(typename Baz::template Part<O>& self, int x) {
    check(x < 1000, "Baz::f: x must be small");
    // A check may reach past its own call's argument into whatever else
    // self/Base exposes -- same liberty Baz::Part<O>::f already has when
    // it calls Base::f(x), just used for a condition instead.
    check(self.remaining() >= x, "Baz::f: not enough budget left");
    auto r = self.f(x);
    check(r < 100000, "Baz::f: result out of range");
    return r;
  }
}

// User-defined, ordinary component, same shape as any hand-written Part.
// Names exactly the methods it wants checked; nothing generic/reusable
// about it beyond ordinary overload resolution picking the right
// contract:: overload for whichever component it wraps.
struct StrictBroker {
  template<typename O>
  struct Part : O {
    using Base = O;
    auto f(int x) { return contract::f(static_cast<Base&>(*this), x); }
    // any method not named here passes straight through, zero cost.
  };
};

int main() {
  std::setvbuf(stdout, nullptr, _IONBF, 0);

  // same Foo, checked and unchecked builds, zero edits to Foo:
  using FooPlain   = APIOf<RawApi, Foo>;
  using FooChecked = APIOf<RawApi, StrictBroker, Foo>;
  FooPlain fp;   std::printf("FooPlain.f(5)   = %d\n", fp.f(5));
  FooChecked fc; std::printf("FooChecked.f(5) = %d\n", fc.f(5));

  // same StrictBroker, unmodified, reused on a totally different component:
  using BazChecked = APIOf<RawApi, StrictBroker, Baz>;
  BazChecked bc;
  // each call sequenced on its own statement -- combining a mutating call
  // and a query call as two arguments of one printf() would leave their
  // relative order unspecified.
  int r1 = bc.f(5); std::printf("BazChecked.f(5) = %d  (budget remaining=%d)\n", r1, bc.remaining());
  int r2 = bc.f(2); std::printf("BazChecked.f(2) = %d  (budget remaining=%d)\n", r2, bc.remaining());

  // Foo's own x!=0 precondition would abort identically for fc.f(0) --
  // not demonstrated here since only one abort can be the last thing a
  // process does; Baz's budget check below is the more interesting one.
  std::printf("violating Baz's budget check on purpose (x alone still passes):\n");
  bc.f(2);   // self.remaining()=1 < 2 -- aborts here, RawApi::f never runs
  return 0;
}
