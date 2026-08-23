/**
 * @file tail_contract.cpp
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief Design-by-Contract via HAPI's Chain<>/Part<> axis: checks and
 *        the adapter applying them live inside one Broker, injected as
 *        the LAST element of the composition list.
 *
 * `Chain<O,OO...>::Part<T> = O::Part<Chain<OO...>::Part<T>>` folds
 * left-wraps-right for any O,OO..., uniformly -- Chain has no notion of
 * "broker" or "anchor" role. Putting `Broker::Contract` last in the list
 * makes it the INNERMOST component, wrapping the real terminal API
 * directly: every call that reaches all the way down passes through it
 * exactly once, regardless of which named component up the chain
 * triggered it. Contrast with strict_broker.cpp, where the broker
 * intercepts each named component's own entry point wherever it sits --
 * a different boundary, not a strictly better version of the same idea.
 *
 * Because Contract sits at the one point every call necessarily passes
 * through, a missing check here means "no check for this method," not
 * "forgot to wire something" -- so this uses an SFINAE soft fallback
 * (if constexpr) instead of strict_broker.cpp's hard compile error.
 */

#include <hapi/hapi.h>
#include <cstdio>
#include <cstdlib>
#include <type_traits>
#include <utility>

using namespace hapi;

inline void check(bool cond, const char* msg) {
  if (!cond) { std::printf("CONTRACT VIOLATION: %s\n", msg); std::abort(); }
}

struct RawApi {
  int budget = 8;
  int f(int x) { budget -= x; return x * 2; }
  int g(int x) { return x + 7; }   // deliberately left unguarded below
  int remaining() const { return budget; }
};

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

struct Qux {
  template<typename O>
  struct Part : O {
    using Base = O;
    int f(int x) { return Base::f(x) - 3; }
  };
};

struct Broker {
  // pre_f takes self (the real Base) as well as the argument -- a check
  // may reach past the one method it's gating into whatever else Base
  // exposes, same liberty an ordinary composed Part already has.
  template<typename O>
  static void pre_f(O& self, int x) {
    check(x != 0, "f: x must not be 0");
    check(self.remaining() >= x, "f: not enough budget left");
  }
  static void post_f(int r) { check(r < 100000, "f: result out of range"); }

  template<typename, typename, typename = void> struct HasPreF : std::false_type {};
  template<typename U, typename O> struct HasPreF<U, O, std::void_t<decltype(U::pre_f(std::declval<O&>(), 0))>> : std::true_type {};
  template<typename, typename = void> struct HasPostF : std::false_type {};
  template<typename U> struct HasPostF<U, std::void_t<decltype(U::post_f(0))>> : std::true_type {};

  // Tail adapter -- must be a nested TYPE exposing its own Part<O>, same
  // shape as Foo/Baz/Qux above (Chain<O,OO...> expects every OO element
  // to look like that, not to itself be the Part template).
  struct Contract {
    template<typename O>
    struct Part : O {
      using Base = O;
      int f(int x) {
        if constexpr (HasPreF<Broker,Base>::value) Broker::pre_f(static_cast<Base&>(*this), x);
        auto r = Base::f(x);
        if constexpr (HasPostF<Broker>::value) Broker::post_f(r);
        return r;
      }
      // g() is not named in Broker -> passes straight through, zero cost.
    };
  };

  // Outer marker -- inert on purpose. strict_broker.cpp's broker does the
  // per-method forwarding at this position; here that work moved to
  // Contract at the tail, so the outer layer has nothing left to do.
  template<typename O>
  struct Part : O {
    using Base = O;
  };
};

int main() {
  std::setvbuf(stdout, nullptr, _IONBF, 0);

  using Checked = APIOf<RawApi, Broker, Foo, Baz, Qux, Broker::Contract>;
  Checked c;

  // each call sequenced on its own statement -- see strict_broker.cpp's
  // comment on why combining a mutating and a query call as two printf()
  // arguments would be a real bug.
  int r1 = c.f(5); std::printf("c.f(5) = %d  (budget remaining=%d)\n", r1, c.remaining());
  std::printf("c.g(5) = %d  (untouched, straight through)\n", c.g(5));
  int r2 = c.f(2); std::printf("c.f(2) = %d  (budget remaining=%d)\n", r2, c.remaining());

  std::printf("violating the budget check on purpose (x!=0 alone still passes):\n");
  c.f(2);   // self.remaining()=1 < 2 -- aborts inside Broker::Contract,
            // before RawApi::f ever runs
  return 0;
}
