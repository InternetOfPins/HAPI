// HAPI x vix.cpp -- opportunity #1 PoC: compile-time cross-module
// dependency checking on top of vix's real, unmodified app-module codegen
// shape. See HANDOFF.md for what's real vs. hand-reproduced.
//
// Meant to be compiled, not run (matches HAPI/tests/compile_tests.cpp's
// own convention) -- everything here is a static_assert / type-level
// check over the generated module classes.

#include <auth/AuthModule.hpp>
#include <db/DbModule.hpp>
#include <app_generated/app_module_chain.hpp>

using namespace vix::app_generated;
using namespace hapi;

// -- the coexistence claim: AppModuleChain sits next to the real
// register_app_modules()/app_generated.cpp, doesn't replace or touch it.
static_assert(AppModuleChain::size == 2, "two modules enabled: auth, db");

// -- the actual win: compiler-enforced cross-module dependency, expressed
// once, checked at every build instead of only when `vix modules check`
// happens to run.
static_assert(query<SameAs<myapp::db::DbModule>, AppModuleChain>,
  "auth module requires db module to be enabled");

// -- the failing case: db module removed from the chain (imagine someone
// disabled it in the manifest). Uncomment to see the real diagnostic:
//
// using AuthOnlyChain = hapi::Chain<myapp::auth::AuthModule>;
// static_assert(query<SameAs<myapp::db::DbModule>, AuthOnlyChain>,
//   "auth module requires db module to be enabled");
//
// Fails with: "static assertion failed: auth module requires db module
// to be enabled" -- one line, naming the actual constraint, not a
// generic type mismatch -- because the message is authored at the
// static_assert site, not derived from a deduction failure.

int main() { return 0; }
