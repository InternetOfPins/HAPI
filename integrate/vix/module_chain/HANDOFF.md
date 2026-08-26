# HAPI × vix.cpp — Opportunity #1 PoC: module dependency chain

LOCAL, not pushed. `HAPI/.RnD/` per the project's own experiment
convention — stays here until measured against a real generated app and
graduated, per the usual `.RnD` → `examples/` rule.

## What this proves

A `hapi::Chain<>` typelist of a vix app's enabled module classes can sit
alongside vix's own real, unmodified codegen (`register_app_modules`)
without touching it, and unlocks a compiler-enforced cross-module
dependency check — a `Q not found` violation becomes a one-line, legible
`static_assert` failure naming the actual constraint, not a generic
type/template error.

## Real vs. hand-reproduced — stated plainly

- **Real**: `HAPIPartners/vix` is a genuine `git clone
  --recurse-submodules` of `https://github.com/vixcpp/vix` (all 30+
  submodules populated). Every template string this PoC's generated-shape
  files reproduce (`ModulesContent.cpp`'s module header template,
  `AppCMakeGenerator.cpp:358-410`'s `register_app_modules` emission loop)
  was read and verified byte-for-byte against that real clone before
  being reproduced here.
- **Hand-reproduced, not CLI output**: `generated_myapp/{auth,db}/
  *Module.hpp` and `generated_myapp/app_generated/*` are **not** the
  output of a real `vix new`/`vix modules add` run. Building vix's own
  CLI standalone (`modules/cli`, its own documented lighter build path)
  hit a real, pre-existing, unrelated bug in vix's own CMake: the `note`
  module's embedded reply runtime can't see the `nlohmann/json.hpp` the
  `json` module already fetched in a sibling build directory
  (`CMakeLists.txt:194`, "[note] missing nlohmann/json.hpp"). Fixable by
  installing the system `nlohmann-json3-dev` package, but that needs
  `sudo` and this session has no passwordless sudo — didn't ask for one,
  fell back per the plan instead. So: the module *files* are faithful,
  manually-typed reproductions of the verified templates (exact class
  shape, exact naming transform — `module_class_name("auth")` →
  `Auth`, confirmed from `ModulesContent.cpp:83-98` — exact generated
  comments), not a literal CLI transcript. Worth re-running for real once
  that CMake bug is fixed upstream or the system package gets installed.
- **Real**: `app_generated.cpp`'s `#include <vix/executor/
  RuntimeExecutor.hpp>` and the whole PoC compile cleanly against the
  **real** `vix/modules/core/include/` headers (not reproduced) — no
  build of `core` needed for this, just real header inclusion.

## A real bug this caught (in the handoff doc itself, not vix or HAPI)

The doc's own proposed code sample —
`static_assert(hapi::Exists<hapi::SameAs<T>, Chain>, ...)` — doesn't
compile as written. `hapi::Exists<Q,Input>` (`meta.h`) is a **type alias**
(`Any<Q>::Check<Input>`, ultimately a `bool_constant<...>`), not a bool —
using it bare in a `static_assert` condition is a parse error ("expected
primary-expression before ','"), caught immediately on first real
compile. `hapi::query<Q,Input>` (`rules.h`) is the actual bool variable
template (`Exists<Q,O>::value`, exposed directly) — same primitive
`Requires`/`Excludes`/this session's `NoCollision` already use. Fixed in
this PoC; worth fixing in the doc/pitch before it's quoted anywhere else.

## Verified results

- `module_chain_poc.cpp`: `AppModuleChain::size == 2`, and
  `query<SameAs<myapp::db::DbModule>, AppModuleChain>` passes (dependency
  present) — real g++ 13.3.0 **and** clang++ 18.1.3, `-std=c++20 -Wall
  -Wextra -Wpedantic`, zero warnings on either, both exit 0.
- Failing case (commented out in the file, `AuthOnlyChain` without
  `DbModule`) verified separately by uncommenting on a scratch copy: one
  clean error — `static assertion failed: auth module requires db module
  to be enabled` — naming the actual constraint, no cascade, no generic
  template noise. Same on both compilers in kind (not separately
  re-checked on Clang for this exact case, but the mechanism is identical
  `static_assert`, no reason to expect divergence given everything else
  matched).
- `app_generated.cpp` (the traditional generated file this coexists with,
  untouched by the Chain<> addition) compiles clean standalone against
  real vix `core` headers, both compilers — confirms the coexistence
  claim, not just the Chain<> in isolation.

## Honest scope of what's NOT proven yet

- vix's actual generator (`AppCMakeGenerator.cpp`) was not modified —
  this proves the emitted shape *would* work, not that vix emits it. That
  step is the real upstream PR, later, per the outreach doc's own
  sequencing (after #2 and #3 also have evidence).
- Not tested against a real, running `vix::App` — `register_routes`/
  `name()` are declared, never defined or called; this is a pure
  compile-time/type-level check, matching `HAPI/tests/compile_tests.cpp`'s
  own "meant to be compiled, not necessarily run" convention. No claim is
  made about runtime behavior.
- Not re-attempted against a real CLI-scaffolded app — see the CMake bug
  above. Worth a follow-up round once that's resolved, to confirm the
  hand-reproduced shape really does match a live `vix modules add` run
  byte-for-byte, not just the generator source.

## Files in this round

- `generated_myapp/auth/AuthModule.hpp`, `generated_myapp/db/DbModule.hpp`
  — hand-reproduced generated module headers.
- `generated_myapp/app_generated/app_generated.hpp`,
  `app_generated.cpp` — hand-reproduced `register_app_modules`, real
  vix's exact emission shape.
- `generated_myapp/app_generated/app_module_chain.hpp` — the actual new
  artifact: what the same generator loop could emit for free, one more
  line per existing iteration.
- `module_chain_poc.cpp` — the consumer PoC, passing + documented failing
  case, compiled on both toolchains.
