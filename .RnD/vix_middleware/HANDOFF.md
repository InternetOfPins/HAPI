# HAPI × vix.cpp — Opportunity #3: opt-in Chain<>::Part middleware

LOCAL, not pushed. Closes out the three-opportunity outreach exploration
(#1 `vix_module_chain`, #2 `vix_json_benchmark`, both this session).

## Real build blocker, tried twice — no real `vix::App` reachable

Same conclusion as opportunity #1, now confirmed a second, independent
way: vix's own CLI/core build hit **two separate, unrelated, pre-existing
CMake bugs** this session. (1) `note` module can't find `json`'s fetched
`nlohmann/json.hpp` across sibling build dirs — worked around locally
with `-DCMAKE_INCLUDE_PATH` pointing at the already-vendored copy, no
sudo needed. Past that, configure genuinely succeeded (`core`, `utils`,
`json`, `websocket`, `p2p`, `note`, all real modules configured clean).
(2) Then the CMake *generate* step failed for a different reason:
`vix_utils`'s install/export set references `fmt-header-only` and
`spdlog_header_only` targets that aren't in any export set — needs
system `fmt`/`spdlog` packages, which needs `sudo` this session doesn't
have, or real surgery on vix's own CMake export configuration, which
isn't this session's place to do on an unmodified external repo. Two
independent real bugs is real signal, not chased a third way — everything
below is a standalone PoC, disclosed as such, not integration-tested
against a real running `vix::App`.

## What was built

Minimal, faithful stand-ins for vix's real, confirmed-exact types
(`Middleware = std::function<void(Request&,ResponseWrapper&,Next)>`,
`Next = std::function<void()>`, `App.hpp:106-111`) — `vix_stand_ins.hpp`.

- `baseline_std_function.hpp` — reproduces `App.cpp`'s real
  `run_middleware_chain_` exactly (recursive walk, fresh `Next` closure
  per layer).
- `chain_middleware.hpp` — the `Chain<>::Part` alternative: same
  semantic middleware roles (auth check, rate limit, log pass-through),
  direct inheritance-resolved calls instead of `std::function` dispatch.
- `stub_app.hpp` + `api_sketch_demo.cpp` — the doc's exact proposed
  call-site shape, `app.get<Chain<Auth,RateLimit>>("/x", handler)`,
  against a minimal stand-in `App` (not real `vix::App`).
- `bench_baseline.cpp` / `bench_chain.cpp` — disassembly/binary-size
  comparison drivers.
- `compile_time_scan.cpp` — compile-time cost at increasing middleware
  depth (N=1,2,4,8), synthetic middleware types.

## Two real bugs found and fixed while building this (not assumed)

1. **My own PoC harness bug**, not HAPI's: `Pipeline p{HandlerTerminal<Handler>{...}}`
   failed to compile — `Chain<>::Part<T>`'s inherited constructors
   (`using Base::Base;`) don't provide a converting constructor *from* a
   base-class object; they propagate the *terminal's own* constructor
   parameter list transparently instead. Fixed by giving `HandlerTerminal`
   a real constructor and building `Pipeline` directly from the handler
   value (`Pipeline p{std::move(handler)};`) — the actual, correct use of
   `using Base::Base;`'s propagation.
2. **A real diagnostics gap**, found and fixed the same way `find<Q>()`
   was fixed this session: a middleware type with no nested `Part<O>` at
   all (the realistic misuse — a data-only struct pasted in by mistake)
   cascaded into the same ~60-line "no matching constructor" candidate-
   overload noise `find<Q>()` used to produce. A leading `static_assert`
   alone wasn't enough — a `using Pipeline = ...` alias instantiates
   `Chain<>::Part<T>` immediately regardless of branch, unlike `if
   constexpr`'s genuinely lazy discarded-statement body. Fixed by moving
   the whole pipeline-construction block *inside* `if constexpr
   (AllMiddlewareShaped<...>::value)` — reusing `hapi::HasPart` (rules.h,
   this session's own `NoCollision` work) rather than a new trait.
   Verified clean, one line, naming the real problem, on **both** GCC and
   Clang, after the fix; confirmed non-regressive on the real, valid
   demo on both too.

   A *different* misuse (a middleware `Part<O>` that simply omits
   `handle()` and inherits it from `Base` unchanged) was tried first and
   turned out to be **correct, not a bug** — ordinary inheritance makes
   it a legitimate no-op pass-through layer, matching this project's own
   established no-op-handler convention elsewhere. Worth noting: not
   every "looks wrong" case is actually wrong.

## Real measurements

**Disassembly / binary size** (`-O2`, g++ 13.3.0, same 3 middleware,
100k-iteration driver in each):

| | baseline (`std::function`) | Chain<>::Part |
|---|---:|---:|
| `.text` size | 7364 B | 1462 B |
| indirect calls (whole binary) | 19 | 2 |
| heap allocation | **yes** — real `operator new`/`operator delete` (`_Znwm`/`_ZdlPvm`), from the `Next` closures' type-erased storage | none |
| middleware pipeline calls | via `std::function::_M_invoke` thunk | **zero** — full 3-layer pipeline inlines directly into `main`, confirmed by disassembly, not assumed |

The heap-allocation finding is the concrete version of what the doc's
source-reading only inferred ("N ephemeral closure constructions") —
confirmed here as literal `new`/`delete` calls, not just an abstract cost.

**Compile-time cost** (the honest, disclosed tradeoff side — a pitch that
only shows wins isn't credible to a team this careful about tradeoffs;
their own JSON benchmark README already models exactly this kind of
both-sides accounting): N=1 averages ~0.628s, N=8 averages ~0.647s (3
runs each) — a real but small (~3%) increase across an 8x depth increase,
dominated by fixed per-translation-unit overhead (HAPI header parsing,
g++ startup), not per-layer template-instantiation cost. Not a real
concern at the depths a middleware chain would realistically use.

## Honest scope

Not integration-tested against real `vix::App` — see the build blocker
above. Proves the composition mechanism, its cost/benefit tradeoff, and
the call-site shape all work and compile cleanly on both toolchains; does
not prove it slots into vix's real router/route-registration path
unmodified. That's the real next step if this becomes an actual
conversation with the vix maintainers — worth raising both CMake bugs to
them directly at that point, not something to keep working around alone.

## Status: all three opportunities now have real, verified evidence

Per the outreach doc's own sequencing (step 4): only now would opening a
GitHub issue/discussion on `vixcpp/vix` be in scope — not done this
round, that's Rui's call, not something to do unprompted.
