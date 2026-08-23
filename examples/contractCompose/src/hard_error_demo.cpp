/**
 * @file hard_error_demo.cpp
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief Deliberately fails to compile. Documents strict_broker.cpp's
 *        "missing contract is a hard error" property -- NOT part of the
 *        normal build (see platformio.ini's env:hard_error).
 *
 * The error fires from COMPOSING the type, not from calling the method --
 * in fact from merely NAMING the type (a bare `using` alias with no
 * object of that type ever instantiated is enough under g++ 13.3.0).
 * GCC-observed specifically; worth re-checking per toolchain before
 * relying on "fails at compose time" as a universal guarantee.
 */

#include <hapi/hapi.h>
using namespace hapi;

struct RawApi { int f(int x) { return x * 2; } };

struct Bar {   // deliberately no contract:: overload anywhere for this one
  template<typename O>
  struct Part : O {
    using Base = O;
    int f(int x) { return Base::f(x) + 100; }
  };
};

namespace contract {
  // intentionally empty -- Bar gets no overload
}

struct StrictBroker {
  template<typename O>
  struct Part : O {
    using Base = O;
    auto f(int x) { return contract::f(static_cast<Base&>(*this), x); }
  };
};

int main() {
  using ShouldFail = APIOf<RawApi, StrictBroker, Bar>;
  ShouldFail s; (void)s;   // fails HERE, even though .f() below is never reached
  return 0;
}

// Expected error (g++ 13.3.0):
//   error: 'f' is not a member of 'contract'
//       auto f(int x) { return contract::f(static_cast<Base&>(*this), x); }
//                                        ^
