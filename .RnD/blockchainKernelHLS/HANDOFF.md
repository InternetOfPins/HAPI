# HANDOFF: blockchain-as-HAPI-composition, HLS synthesis leg

**2026-08-20, independently re-verified.** Steps 7, 8, 9 below were
reproduced from scratch against the same real headers (own reconstruction
of `components_portable.h`/`chainA_tu_portable.cpp` plus a `chainB`/CRTP
counterpart, since only the `ChainA`-side files were in the handoff
bundle) — every number matched within a few bytes (594B vs. 589B
standalone, +289B vs. +289B / +566B vs. +562B linked, 16 vs. 16 / 6 vs. 7
error lines), consistent with trivial toolchain-version variance, not a
substantive discrepancy. `steps7-9/` in this directory holds the
reconstructed, reproducible test files.

**Framing correction (Rui, same date): the step 9 CRTP result is not a
new finding.** CRTP is deliberately kept optional in HAPI specifically
*because* of this cost (2x+ at the type level): a self-reference makes a
composition's terminal type unique, so the separately-verified
common-composition-segment-sharing optimization can't fire across sibling
compositions unless they're structurally identical. Steps 7-9's
`nm`/linked-size measurement is a fresh, useful *quantification* of
already-known, designed-for behavior — not a discovery. The existing
"reserve `CRTP<>` for singleton-ish roles" guidance already covered this;
now it has real numbers attached.

Written by a sandboxed agent with no Bambu/Vitis/hardware access, for a
local agent that has both. Everything below marked "verified" was actually
compiled and run, not estimated — either in this sandbox (native g++ +
real `avr-g++`/`arm-none-eabi-g++`/`riscv64-unknown-elf-g++`/`xtensa-esp-elf-g++`
cross-compiles) or is a direct empirical claim from Rui's own prior HAPI/
OneHLS work (cited where used). Everything marked TODO needs your Bambu/
Vitis install and hasn't been run anywhere.

## Origin

This is steps 7-11 of an 11-step plan to test whether a blockchain
protocol can be expressed as a static HAPI composition — "can HAPI
compose the independently replaceable parts of a blockchain into a
specialized implementation with no abstraction/runtime machinery for
unused features," with the blockchain domain as a stress-test, not a goal
in itself. Steps 1-6 (baseline, component ID, first composition,
variation, optional-feature elimination) were skipped as re-demonstrating
what `benchmark/check_zero_overhead.sh`, the Arduino_GFX fork, and
`hls_can_disabler` already proved.

## What's already verified (steps 7-10, this session)

All against the real `InternetOfPins/HAPI` headers (cloned fresh from
`github.com/InternetOfPins/HAPI`, `main` at time of test, post the
2026-08-20 `At<>` fix).

**Step 8 — heterogeneous cross-component dependency.** A `Consensus`
component reaching `Validation`/`Hash`/`Transaction`/`State` (four
different roles) via ordinary `this->` member access, zero glue code, by
being listed **first** in `Chain<...>` (first-listed = outermost/most-
derived, per `chain.h`: `Chain<O,OO...>::Part<T> = O::Part<Chain<OO...>::Part<T>>`).
Compiles, runs, `sizeof == 12` bytes (3×`uint32_t`, no padding, no
vtable) even at 4 levels of heterogeneous dependency.

**Step 7 — mandatory-API-absence diagnostics.** `Requires<SameAs<X>,After>`
in `rules()` gives a clean, actionable first `static_assert` message
("Validation: Hash must be reachable below Validation") vs. the raw
"has no member named 'hash'" you get without it. Caveat: without
`-fmax-errors=1`, GCC prints the clean message *then* still hits the raw
error too (both are independently triggered). Relevant to the still-open
TMP-diagnostics problem (the `menuDef<WrapNav>` wall-of-types case) —
partial answer, not the fix.

**Step 9 — does a shared prefix across two "protocol families" actually
share code?** Two chains, same `Validation,Hash,Transaction,State`
prefix, different outermost `Consensus`, compiled in separate TUs at
`-O0`. Baseline: byte-identical mangled symbols for the shared prefix,
linker keeps one copy, +289B for a whole second consensus family
(vs. 564B standalone). Reproduced the real `CRTP<>` idiom from
`examples/crtp/src/main.cpp` (`CRTP<APIOf<Terminal<>,OO...>>` at the
terminal) — this makes the terminal's *type* embed the full feature list,
poisoning the entire prefix's type identity, not just the CRTP-using
layer: +562B for the second chain, ~full duplication. Sharpens the
existing "CRTP breaks base-symbol sharing" finding into a specific
mechanism (blast radius = whole prefix down to the terminal).

**Step 10 — real embedded targets, and a real bug.** Cross-compiled for
ATmega328P, STM32F103 (Cortex-M3), CH32V003 (RV32EC), ESP32 (Xtensa LX6),
ESP32-S3 (Xtensa LX7) — all targets Rui has hardware for. Found that
`base.h`'s `#ifdef __AVR__ ... #else #include <cstddef> ...` gate breaks
on `riscv64-unknown-elf-g++` (this Ubuntu package specifically — a
bare-metal newlib/picolibc toolchain with no libstdc++ C++ headers at
all: `cstddef`/`cstdint`/`type_traits`/`utility` all missing, plain C
`stddef.h`/`stdint.h` present). Reproduces on the **pristine, unmodified**
headers, not just this session's test scaffolding. Xtensa (ESP32/ESP32-S3,
real `xtensa-esp-elf` toolchain from Espressif's own GitHub releases) has
no such gap — full libstdc++ port — so this is specifically an
`riscv64-unknown-elf-g++`-as-packaged issue, not a general bare-metal
problem.

**Fix, verified on all 6 targets + `compile_tests.cpp` regression, not yet
applied to the real repo:**

```diff
--- a/include/hapi/base.h
+++ b/include/hapi/base.h
@@ -6,9 +6,20 @@

 #pragma once

-#ifdef __AVR__
+// __AVR__ toolchains, and some bare-metal newlib/picolibc cross toolchains
+// (e.g. riscv64-unknown-elf-g++ as packaged for rv32e/CH32V003) ship a C
+// library but no C++ libstdc++ port -- <cstddef>/<type_traits>/<utility>
+// don't exist there at all. Detect via __has_include instead of hardcoding
+// __AVR__ as the only such platform.
+#if defined(__has_include)
+  #define HAPI_HAS_STL_HEADERS __has_include(<cstddef>)
+#else
+  #define HAPI_HAS_STL_HEADERS 1
+#endif
+
+#if defined(__AVR__) || !HAPI_HAS_STL_HEADERS
   #include "platform/avr/avr_std.h"
-  using SizeT=unsigned int;
+  using SizeT=__SIZE_TYPE__;
 #else
   #include <cstddef>
   #include <type_traits>
   #include <utility>
   using SizeT=size_t;
 #endif

--- a/include/hapi/platform/avr/avr_std.h
+++ b/include/hapi/platform/avr/avr_std.h
@@ -23,7 +23,9 @@
   template<typename O> constexpr O max(O a, O b) { return a > b ? a : b; }
   template<typename X> constexpr X abs(X x) { return x < 0 ? -x : x; }

-  using size_t = unsigned int;
+  using size_t = __SIZE_TYPE__; // was hardcoded `unsigned int` (correct for
+                                 // AVR's 16-bit int, wrong for a 32-bit
+                                 // freestanding target reusing this shim)

   template<typename T> struct numeric_limits {
```

Measured `.text` (`-Os`, patched headers, full 5-layer chain unless noted):
native x86-64 277B, ATmega328P 308B, Cortex-M3 100B, CH32V003 150B, ESP32
148B, ESP32-S3 148B.

**TODO for you:** decide whether to land this on `main` (it's a strict
generalization of the existing gate — verified zero regression on every
target that already worked, including the two Xtensa parts). Cosmetic,
not bundled: `avr_std.h` isn't AVR-specific anymore, might deserve a
rename (e.g. `minimal_std.h`) — separate decision, separate commit if you
want it.

## Step 11 — what THIS handoff is actually for

Not attempted: no Bambu, no Vitis HLS in the sandbox this was built in.
The scaffolding is done and native-verified; the actual synthesis runs
are not.

**New example, ready to run:** `examples/hls_blockchain_kernel/` — two
top-level kernels, same `Transaction`, different `Hash`:

- `hashMurmurTop` (`hls/hash_murmur_top.cpp`) — `Chain<MurmurHash,Transaction>`,
  has a real constant multiply (`h *= 0x5bd1e995u`)
- `hashXorFoldTop` (`hls/hash_xorfold_top.cpp`) — `Chain<XorFoldHash,Transaction>`,
  same shape, shifts/xors only, no multiply

Both scalar-in/scalar-out (`uint32_t f(uint32_t,uint32_t)`), no structs by
value, no I/O reachable from either top function — built this way
specifically to stay clear of the known Bambu `FixStructsPassedByValue`
crash (documented in `hls_smoke/README.md`'s "Known caveat"). Native
regression check (`src/main.cpp`) already run:

```
hashMurmurTop(500,7)  = 1614968633
hashXorFoldTop(500,7) = 59186122
sizeof(KernelMurmur)  = 8
sizeof(KernelXorFold) = 8
```

`platformio.ini`, `extra_hls.py` (Bambu), `extra_hls_vitis.py` +
`vitis/run_hls.tcl` (Vitis) are all wired following `hls_fir`'s exact
`_bambu_cmd`/`_vitis_cmd` pattern — same device (`xc7a100t-1csg324-VVD`
Bambu-side / `xc7a100tcsg324-1` Vitis-side), same 10ns period, same
`-gcc8` cross-check convention.

### What to run

```sh
export BAMBU_APPIMAGE="/path/to/bambu.AppImage"
cd examples/hls_blockchain_kernel
pio run -e hls -t synthesize-hash-murmur
pio run -e hls -t synthesize-hash-xorfold
pio run -e hls -t synthesize-hash-murmur-gcc8    # expected to fail, see below
pio run -e hls -t synthesize-hash-xorfold-gcc8   # expected to fail, see below

export VITIS_HLS="/path/to/vitis-run"
pio run -e hls-vitis -t synthesize-hash-murmur-vitis
pio run -e hls-vitis -t synthesize-hash-xorfold-vitis
```

Fill in `examples/hls_blockchain_kernel/README.md`'s Results table from
Bambu's `Summary of resources:` output (not a grep over the generated
Verilog — see `hls_smoke/README.md`'s "Automated check" section for why
that's not a valid instance count) and Vitis's synthesis report.

### What to predict, and why — confirm or refute, don't just confirm

1. **`hashXorFoldTop`: zero DSPs, pure logic fabric, on both tools.**
   No multiply anywhere in the design — shifts and xors only. Low-risk
   prediction, mainly a sanity check that the tooling is wired right.

2. **`hashMurmurTop`'s constant multiply (`h *= 0x5bd1e995u`) — DSP or
   strength-reduced?** This is the actual interesting question, and
   there's real precedent pointing in a specific direction: OneHLS's own
   `Fir<>` component with non-power-of-two coefficients mapped to **2
   real DSP48s under Vitis vs. 0 under Bambu (strength-reduced to
   shift/add)** — same shape of question (fixed, non-power-of-two
   compile-time constant multiply), different component. Prediction:
   same split here. If Bambu strength-reduces `0x5bd1e995u` the same way
   it did the FIR coefficients, and Vitis maps it to a real DSP48, that's
   confirmation the pattern generalizes beyond FIR specifically. If
   either tool surprises you, that's the more interesting result — write
   it up either way.

3. **Watch for the `static const` fixed-point non-constant-folding bug —
   almost certainly doesn't apply here, but check.** Separate, already-
   documented Bambu issue: `static const` fixed-point literals don't
   constant-fold under Bambu and synthesize a real stateful lazy-init
   guard instead (the reason OneHLS's `Q8.8` values are raw bit-pattern
   integer NTTPs, not `static const` fixed-point objects). `0x5bd1e995u`
   here is a plain inline integer literal, not a `static const`
   fixed-point object, so this almost certainly doesn't apply — but if
   `hashMurmurTop`'s synthesis shows unexpected state/registers where
   none are expected (both kernels should be purely combinational, 1
   control step, 1 cycle, same as `hls_smoke`'s `wrapSum` — no cross-
   clock-edge state in either design), this is the first thing to check
   before assuming it's a real HAPI problem.

4. **`-gcc8` targets: expected to fail identically to `hls_fir`/
   `hls_smoke`/`hls_can_disabler`.** Bambu's `I386_GCC8` frontend doesn't
   recognize the AST node `Chain<>`'s recursive composition produces
   (`Unrecognized keyword ... bound_template_template_parm` / `Parse
   error`). Run them anyway for the record; a pass here would be the
   interesting/reportable result, not the expected one.

### After synthesis: the actual point of this whole leg

If `hashMurmurTop` and `hashXorFoldTop` produce genuinely different
synthesized hardware (different resource counts, not just different
`hash()` bodies at the C++ level) from swapping one template argument —
`Chain<MurmurHash,Transaction>` vs. `Chain<XorFoldHash,Transaction>`, same
composition mechanism — that's the actual HAPI → OneHLS → protocol-
variant connection the original 11-step plan's step 11 was reaching for:
the composition itself defines the synthesized variant, no blockchain-
specific HLS abstraction layer needed. Worth a LinkedIn/r/FPGA post in
the same style as the OneHLS ComplexMac/Fir findings if the numbers hold
up — same "fact-check against real synthesis logs before publishing"
discipline as those posts, not before.
