# Handoff response: HLSLibs AC Datatypes × Bambu HLS (round 3: HAPI component shell)

## Round 5: cascaded-biquad IIR — the deferred stretch goal, now done

Picked up from the scope-cut list: build a real IIR (feedforward +
feedback state, genuinely different from FIR's one-directional shift
register) using `Reg<T>` for both roles, per the `hls_fir/README.md`
stretch-goal note. Synthesized under Bambu, same device/clock as
everything else in this investigation.

**Design, direct form I** (chose form I over the more resource-efficient
direct form II specifically so feedforward and feedback each get their
own visible `Reg<T>`-based tap chain, rather than folding both into one
combined state struct):
`y[n] = b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]`, with
`b1=0.5, b2=0.25, -a1=0.5, -a2=-0.25` — all exact powers of two, chosen
deliberately so the impulse response is exactly hand-computable with zero
rounding. Feedforward taps (`FFTapLogic`+`Reg<Sample>`) are the exact
existing `Tap`/`Reg<Sample>` shape from `fir_lpf4_actypes_reg_top.cpp`,
unchanged. Feedback taps (`FBTapLogic`+`Reg<Sample>`) are new: they
contribute to the sum via `fbSum()` using their *old* delayed output, but
their delay line only shifts via a separate `fbPush()` call once the
final output is known — the genuinely two-phase part an IIR needs that a
FIR's single forward accumulate-and-update pass never does. Both method
pairs (`mac()` for feedforward, `fbSum()`/`fbPush()` for feedback) coexist
in **one flat mono_block-composed chain** — confirmed empirically that
ordinary C++ single-inheritance lookup transparently routes each call
past whichever tap type doesn't define that method, no HAPI-level
"routing" needed: `Chain<FFTap<...>, FFTap<...>, FBTap<...>, FBTap<...>>`.

**Real finding, not assumed:** a zero-fraction-bit `ac_fixed<W,W,true>`
(this investigation's usual convention, safe for FIR since it has no
feedback) genuinely cannot express a stable feedback coefficient — any
nonzero integer has magnitude ≥1, guaranteeing unbounded growth. Real
fraction bits are unavoidable here, which surfaced a second real finding:
**a `static const Sample coeff = 0.5;` (the natural way to write a
fixed-point constant from a double) does NOT constant-fold under Bambu —
it synthesizes a genuine stateful lazy-init guard** (confirmed via the
final resource summary: an extra `register_SE` + `read_cond_FU`/`MUX_GATE`
appear that aren't present in the working version), because the static
local's "has this run yet" semantics get taken literally as real
hardware, not optimized away. The fix: represent each coefficient as an
integer NTTP holding the **raw Q8.8 bit pattern**, reconstructed via
`ac_fixed::set_slc(0, ac_int<16,true>(RawBits))` — a pure bit copy, no
value-conversion logic, confirmed via the same resource-summary check to
introduce neither the lazy-init guard nor any real floating-point
functional unit (the `STD_SOFT_FLOAT` library-availability line in
Bambu's log is boilerplate present in every ac_fixed.h-based design
regardless of whether floating point is actually used anywhere in the
final `Summary of resources` — checked directly, not assumed).

**Verified, native (bit-exact, zero rounding — all chosen values are
exact powers of two):**
- Single section, impulse response (raw Q8.8 units):
  `0 128 128 32 -16 -16 -4 2`.
- Cascade (two independent, already-closed section instances, output of
  stage 1 fed as stage 2's input — same "plain function composition, not
  `Chain<>`-splicing" shape as `fir_lpf_cascade2_top.cpp`, for the exact
  same reason: concatenating tap lists would thread one accumulator
  across taps sharing delayed copies of the same input, which is correct
  for one filter's internal topology but wrong for cascading independent
  sections): `0 0 64 128 96 16 -28 -24` — matches the hand-computed
  convolution of the single section's impulse response with itself
  exactly, cross-checking the direct simulation against the independent
  LTI-system-theory prediction.

**Verified, Bambu** (`xc7a100t-1csg324-VVD --clock-period=10`,
`I386_CLANG16`, `AC_VERSION` guard active): both targets synthesize
clean, zero warnings, real RTL. `biquadTop`: FF=101, area=6855, 0 DSPs,
control steps=5. `biquadCascade2Top`: FF=189, area=12796, 0 DSPs, control
steps=5 — **0 DSPs for both is explained, not just reported**: every
coefficient (128, 64, -64) is a power of two, so Bambu strength-reduces
every multiply to a shift (confirmed in the final resource summary:
heavy `ui_lshift_expr_FU`/`ui_rshift_expr_FU` presence, matching the
same shift-strength-reduction behavior already seen for the original
binomial FIR taps). Cascade doesn't scale a clean 2x over the single
section (FF 1.87x, area 1.87x) — plausibly the two dispatch-method
pairs' shared control/scheduling overhead amortizing across the larger
design, not investigated further (structural correctness is what this
round set out to prove, not resource-scaling precision).

**Files:** `HAPI/.RnD/acTypesHLS/hls/biquad_top.cpp`,
`hls/biquad_cascade2_top.cpp`, `tests/biquad_native_test.cpp`.
`run_tests.sh` extended (native biquad test + 2 new Bambu targets) — now
17/17 passing. No `docs/`/`README.md`/paper changes this round, per
explicit instruction ("no paper concern on this phase").

## Round 4 (2026-08-16, same day): `Reg<T>` promoted into HAPI core

`hapi_reg.h`'s content moved to `HAPI/include/hapi/reg.h` verbatim (no
functional changes, zero includes needed — it's a self-contained
template), wired into `hapi.h`'s aggregation
(`#include "hapi/reg.h"`) so plain `#include <hapi/hapi.h>` picks it up.
The `.RnD/acTypesHLS/` local copy was deleted; the three targets that
included it now just rely on `<hapi/hapi.h>`'s aggregation. Re-ran the
full `run_tests.sh` suite (14/14) and the zero-overhead diff after the
move — byte-for-byte identical Bambu numbers, confirming the promotion is
purely organizational, not semantic. Also added a standalone, AC-types-
independent regression case to `HAPI/tests/compile_tests.cpp` (a
`Counter`/`Reg<int>` demo proving state persists across calls: `1,2,3`)
so `Reg<T>` has real core-level test coverage, not just AC-types-specific
proof. No `docs/`/`README.md` changes in this pass — that's still a
separate, not-yet-decided step.

## Round 3: can AC Datatypes be wrapped by a reusable HAPI Chain component?

### Goal (round 3, as given)
Round 2 proved a plain `-I` include (any order, `AC_VERSION`-guarded) is
enough to use real upstream ac_types safely. But `fir_lpf4_actypes_top.cpp`
still hand-declares its delay register as a raw member (`Sample z{0};`) — it
never tested whether AC Datatypes values can be held by a *reusable* HAPI
Chain-composable component. Also asked: are Siemens' AC Datatypes components
"already a composition result," or independent pieces to compose ourselves?

### On AC Datatypes' own composition (direct answer, from reading the real
### headers, not guessing)
AC Datatypes composes at the **plain-C++ value level**, never CRTP/mixin like
HAPI: `ac_complex<T>` is literally `struct { T _r; T _i; }` — two fields of
the same leaf type, explicitly documented to accept `ac_int`/`ac_fixed`/
`ac_float`/C-native types, mixed. `ac_fixed`/`ac_int` are sibling types
privately inheriting a shared low-level storage primitive (`ac_private::iv<N>`)
— not parent/child, despite `ac_fixed.h` `#include`-ing `ac_int.h`.
`ac_shared<T>` is a thin forwarding wrapper, but for a *different* purpose
(Catapult multi-process sharing intent, not state). `ac_channel<T>`/
`ac_split_join.h` are SystemC-testbench FIFO infrastructure (`#include
<deque>`), not synthesizable datapath types — ruled out of scope. `ac_reg.h`
isn't a type at all — it's a 10-line identity function, a Catapult scheduling
*pragma-hint* ("insert a register here"), nothing to wrap.

**Conclusion: HAPI doesn't need to understand AC's internal composition at
all.** One generic shell that treats any AC type as an opaque value `T`
(atomic like `ac_int`, or itself composed like `ac_complex<ac_fixed<...>>`)
covers the whole family for free — AC types build richer *values*, HAPI
Chains build the *structure/behavior* around those values. Complementary,
not competing, paradigms.

### `Reg<T>` — the shell
```cpp
namespace hapi {
  template<typename T>
  struct Reg {
    using Type = T;
    template<typename I>
    struct Part : I {
      using Base = I;
      using Base::Base;
      T value{};
    };
  };
}
```
One type parameter, no shell-level NTTPs (each AC type already fully absorbs
its own NTTPs into a type alias before reaching the shell). Raw public
member, not `get()`/`set()` — zero-overhead is the whole point. Lives at
`HAPI/.RnD/acTypesHLS/hapi_reg.h`, prototype only, **not** promoted into
`HAPI/include/hapi/` (that's a later, separate decision).

Uses HAPI's already-proven **mono_block** feature (`Chain<X,Y>` usable as one
element inside another `Chain`, `.RnD/mono_block/`, tests passing): logic and
state split into two separately-composed pieces, `Tap<Coeff> =
Chain<TapLogic<Coeff>, Reg<Sample>>`, dropped unmodified into the outer
`Chain<Tap<10>, Tap<118>, ...>`. Ordering matters here (re-derived from
`chain.h`'s actual `Part` collapse, not assumed): the first-listed component
is outer/derived-most, so **logic must come before state** for the logic
layer to reach `value` via ordinary inheritance (`Base::value`).

### Three new targets, all in `HAPI/.RnD/acTypesHLS/`
- **`hls/fir_lpf4_actypes_reg_top.cpp`** — the existing FIR filter,
  refactored to use `Reg<Sample>` instead of a raw member. **Zero-overhead
  claim, empirically confirmed byte-for-byte**: FF=62, Registers=2,
  Area=7679, DSPs=0, control steps=4 — identical to the raw-member baseline
  (`examples/hls_fir/hls/fir_lpf4_actypes_top.cpp`) on every metric. Native
  impulse response also identical: `0 10 118 118 10 0 0 0`.
- **`hls/ac_accumulator_top.cpp`** — genuinely stateful `ac_int<8,true>`
  accumulator (unlike round 2's purely-combinational `ac_int_ops_top.cpp`),
  proving `Reg<T>` holds cross-call state correctly, including wraparound:
  `+50,+50,+50` → `50, 100, -106` (hand-verified, matches). Synthesizes
  clean: FF=32, area=1880, 0 DSPs, no warnings.
- **`hls/ac_complex_cmac_top.cpp`** — `ac_complex<ac_fixed<16,16,true>>`
  complex multiply-accumulate, proving the shell handles an
  AC-internally-composed type exactly like an atomic one, zero extra HAPI
  code. Coefficient `2-1j`; input `3+4j` → step 1 `10+5j`, step 2 `20+10j`
  (hand-verified via the standard complex-multiply formula, matches).
  Synthesizes clean: FF=64, area=3943, 0 DSPs, control steps=6. One
  benign, expected warning (`This function uses unknown addresses`) —
  the function's `int32_t* out_re, out_im` pointer output params are the
  first pointer-argument design in this whole investigation, and Bambu
  correctly infers a runtime-address memory port for them; not a defect
  (confirmed: clean RTL generated, no other warnings, native output
  correct).

**Bug caught during implementation, not by luck:** the first cut of the
complex-mac target used two separately-callable top-fns (`cmacReTop`/
`cmacImTop`) sharing one static accumulator, each independently calling
`.step()` — calling both from the same process silently double-advances the
shared state (compounded by C++'s unspecified function-argument evaluation
order, which reordered which "step" each result actually reflected in a
`printf` harness). Harmless for Bambu itself (each `--top-fname` run only
reaches its own function, no cross-run state), but a real footgun and a
native-testing trap. Fixed: one top-fn, one `.step()` call, both parts out
via pointers (`cmacTop(xr, xi, &out_re, &out_im)`). Also hit and fixed two
ordinary "most vexing parse" ambiguities (`Type(Args)` inside constructor
argument lists parsed as declarations) by switching to brace-init.

### Test suite extended (`run_tests.sh`, now 14/14 passing)
Added: a second native-test binary (`tests/hapi_reg_native_test.cpp`,
linked directly against all 4 real `hls/*.cpp` targets, not a
reimplementation — confirmed empirically that linking the public baseline
file and the three new `.RnD` targets together in one binary has no ODR/
symbol conflicts); the 3 new targets in the Bambu synthesis loop (with an
explicit, documented exception for the complex-mac target's one expected
warning); and a new automated zero-overhead diff step that `grep`s FF/
Registers/Area/DSP/control-steps from both FIR variants' logs and asserts
they match exactly, rather than eyeballing two log files.

### Not done (explicit scope cuts, per round 3's handoff)
1. `ac_shared<T>`'s behavior under Bambu — different semantic axis
   (concurrency-sharing intent, not register/state), unchecked, separate
   investigation.
2. Promoting `Reg<T>` into `HAPI/include/hapi/` — stays a `.RnD/`-only
   prototype; that decision is for later.
3. No README.md/paper changes.
4. No composing full DSP filter blocks beyond these 3 MVP examples (a
   cascaded-biquad IIR using `Reg<T>` for both feedforward/feedback taps
   would be a good, structurally-different follow-up — `hls_fir/README.md`
   already flags biquad as an unstarted stretch goal), no MatchLib
   Connections comparison.

---

# Round 1 & 2 (original text below)

## Goal (round 2, as given)
Round 1 confirmed real upstream `ac_types` synthesizes on Bambu but found a
trap (Bambu bundles its own older fork on its default include path). Round
2's job: turn that single hand-checked example into a real test suite, and
directly answer whether AC Datatypes can be pulled into HAPI/IOP work via a
**plain, simple include** — no ordering ritual, no per-target ceremony — or
whether safe use genuinely requires something more. Report the verdict
plainly either way; don't force a false positive.

## Verdict: **simple include works.** One `-I` flag, any position, is
## sufficient — provided a 2-line compile-time guard is always present.

The only real failure mode, confirmed by direct experiment (not
speculation), is **omitting `-I` for the third-party header entirely** —
which is true of literally any third-party library and isn't a special
AC-types/Bambu gotcha. Ordering between multiple correct `-I` flags does
**not** matter. Round 1's "must precede HAPI_INC" fix was overcautious —
correct in effect (it worked) but based on the wrong mechanism. See finding
R2-1 below for the direct test that overturned it.

## Round 2 findings

**R2-1. Include-order matrix, run empirically, not assumed.** Took the
existing `fir_lpf4_actypes_top.cpp`, added a 2-line compile-time guard
(see R2-2), and ran it through Bambu at the same device/clock three ways:

| Arrangement | Result |
|---|---|
| `-I<ac_types> -I<HAPI>` (ac_types first) | **PASS**, FF=62/area=7679 (real upstream numbers) |
| `-I<HAPI> -I<ac_types>` (ac_types last) | **PASS**, FF=62/area=7679 — **identical** to the row above |
| `-I<HAPI>` only (ac_types omitted) | **FAILS** — hard compile error from the guard, exit 11 |

Ordering between two *correct* `-I` flags is irrelevant — Bambu's clang-16
frontend correctly prioritizes any user-supplied `-I` over its own internal
default include directory regardless of position. The only way to actually
hit round 1's trap is to forget the flag altogether. Also worth stating
plainly since round 1 speculated about it: **PlatformIO's own include
injection is not a factor here at all** — the custom Bambu targets in
`extra_hls.py` shell out to `bambu` directly (`cd outdir && bambu -I... file.cpp`),
completely bypassing PlatformIO's own compiler invocation. That hypothesis
never applied to begin with.

**R2-2. A version-macro guard, not a header patch, is the real safety
net.** Round 1's verification trick (planting a `#warning` inside a patched
copy of `ac_int.h`) required editing the third-party header — not
reusable, not something a future example author would know to do. Better:
the real upstream header and Bambu's bundled fork already disagree on their
own version macros, unpatched:
- Bambu's bundled fork (`usr/share/panda/ac_types`, Mentor Graphics 3.7.2,
  2017): `#define AC_VERSION 3` / `AC_VERSION_MINOR 7`.
- Real upstream (2026.2, Siemens): `#define AC_VERSION 4` / `AC_VERSION_MINOR 8`.

So the whole safety net collapses to two lines, added directly to any file
that uses ac_types under Bambu, no plumbing required:
```cpp
#include <ac_int.h>   // or <ac_fixed.h>, which itself includes ac_int.h
#if AC_VERSION < 4
#error "resolved to bambu's bundled ac_types fork, not real upstream"
#endif
```
This is portable (works for anyone, no patched clone to distribute) and
was confirmed to fire correctly both as a synthesis-time guard (R2-1's
third row) and as a native-compile negative control (R2-4).

This also gives a concrete, checkable explanation for round 1's finding #3
(real upstream ac_fixed costing ~2x the FF/registers of Bambu's fork at
identical bit widths): Bambu's fork has real `#if defined(__BAMBU__)` /
`#if !defined(__BAMBU__) || defined(__BAMBU_SIM__)` branches throughout
(`__BAMBU__` is predefined by the bambu binary itself during compilation)
selecting simpler, synthesis-friendly code paths. Real upstream has no
knowledge of `__BAMBU__` at all and always takes the generic path — a
real, structural reason for the resource gap, not a curiosity. Not chased
further into the generated RTL (still out of scope per round 1's cut).

**R2-3. Test suite built and passing, covering ground the FIR check
never touched.** New files under `HAPI/.RnD/acTypesHLS/`:
- `tests/ac_types_native_test.cpp` — native (plain g++, no Bambu)
  correctness via `assert()` against known-good values: `ac_int` unsigned/
  signed wraparound, single-bit types, auto-widened add/multiply (no
  overflow), static bit-slice reads; `ac_fixed` with **real fraction
  bits** (`ac_fixed<8,4,true>`, unlike the FIR check's zero-fraction-bit
  swap), `AC_TRN` vs `AC_RND` rounding on a non-exactly-representable
  value (1.05), `AC_WRAP` vs `AC_SAT` overflow on an out-of-range
  assignment (10.0 into a `-8..7` range). All assertions passed on first
  run against real reference values (hand-computed, not guessed after the
  fact).
- `hls/ac_int_ops_top.cpp` (`acIntOpsTop`) — auto-widened multiply/add plus
  a static `slc<>()` bit-slice read, runtime input port. Synthesizes
  clean; Bambu infers a real DSP for the 8×8 signed multiply (1 DSP,
  area 52) — first time this investigation has seen Bambu map an AC
  Datatypes operation to a physical multiplier rather than shift-add.
- `hls/ac_fixed_frac_top.cpp` (`acFixedFracTop`) — `ac_fixed<8,4,true,
  AC_RND,AC_SAT>`, real fraction bits, runtime input port, deliberately
  drives values that overflow the format so `AC_SAT` actually fires
  (hand-verified: input 100 saturates to 7.9375 before the rest of the
  arithmetic runs). Synthesizes clean, 0 DSPs, area 450, FF 94.
- `ac_upstream_guard.h` — the R2-2 guard as a reusable 12-line header for
  this investigation's own test files (inlined directly, not referenced
  via this header, in `fir_lpf4_actypes_top.cpp` itself, since that file
  lives in the public `examples/` tree and shouldn't reach into `.RnD/`).
- `run_tests.sh` — runs all of the above (native tests, the negative
  control, all three synthesis targets, and the full R2-1 include-order
  matrix) as one command, exits non-zero on any unexpected result:
  ```
  BAMBU_APPIMAGE=/path/to/bambu.AppImage \
  AC_TYPES_INCLUDE=/path/to/ac_types/include \
    HAPI/.RnD/acTypesHLS/run_tests.sh
  ```
  Full run, this session: **9/9 checks PASS**, exit 0.

**R2-4. Negative control confirms the guard, not just the ordering
experiment.** Extracted the Bambu AppImage
(`--appimage-extract`), compiled `ac_types_native_test.cpp` natively
against `usr/share/panda/ac_types/include` instead of the real clone: hard
compile error, `AC_VERSION < 4` guard fires exactly as designed. This is
what `run_tests.sh` step 2 automates.

## `extra_hls.py` correction
Removed the now-disproven "AC_TYPES_INCLUDE must precede HAPI_INC" comment
and ordering logic (`_bambu_cmd`'s `extra_include="AC_TYPES"` path now just
appends the flag, order doesn't matter) — replaced with a comment pointing
at this file for the real explanation. The `AC_TYPES_INCLUDE` env-var gate
itself is kept (still the only way to tell the script where a clone lives
on a given machine — not vendored in-repo), but it's no longer framed as
solving an ordering problem it never actually needed to solve.

## Environment / how to reproduce
- Bambu binary and AC Datatypes clone: same as round 1, not cached across
  sessions — re-fetch both (`bambu-2024.10.AppImage`,
  `git clone --depth 1 https://github.com/hlslibs/ac_types`).
- Run the whole suite: `AC_TYPES_INCLUDE=... BAMBU_APPIMAGE=... ./run_tests.sh`
  from this directory.
- The one existing PlatformIO-wired target
  (`synthesize-fir-lpf4-actypes` in `HAPI/examples/hls_fir/extra_hls.py`)
  still works exactly as before — `run_tests.sh` re-verifies its numbers
  independently via a raw `bambu.AppImage` invocation, not through pio.

## Artifacts
- `HAPI/.RnD/acTypesHLS/ac_upstream_guard.h` — new
- `HAPI/.RnD/acTypesHLS/tests/ac_types_native_test.cpp` — new
- `HAPI/.RnD/acTypesHLS/hls/ac_int_ops_top.cpp` — new
- `HAPI/.RnD/acTypesHLS/hls/ac_fixed_frac_top.cpp` — new
- `HAPI/.RnD/acTypesHLS/run_tests.sh` — new
- `HAPI/examples/hls_fir/hls/fir_lpf4_actypes_top.cpp` — modified (added
  the R2-2 guard inline)
- `HAPI/examples/hls_fir/extra_hls.py` — modified (ordering-logic
  correction above)
- All of the above are untracked/uncommitted (confirmed present and intact
  at the start of this round before assuming a clean baseline, per the
  round-2 handoff's instruction).

## Not done (explicit scope cuts, unchanged from round 1)
1. No composing AC DSP filter blocks via `Chain<>`, no MatchLib
   Connections vs. OneIO comparison.
2. Round 1's ~2x FF/register cost now has a plausible *mechanism*
   (R2-2's `__BAMBU__` branches) but wasn't chased into the actual
   generated RTL to confirm — still an open, separate question.
3. No README.md or paper updates — report back for a scope decision, per
   both rounds' handoffs.
4. Nothing committed to git.
5. No RTL simulation of any target (native + resource/synthesis checks
   only, same limitation as every prior HLS round in this repo).

## Links
- HLSLibs ac_types: https://github.com/hlslibs/ac_types
- Bambu prebuilt AppImage: https://release.bambuhls.eu/bambu-2024.10.AppImage
- Round 1 findings folded into this same file above; see git history of
  this file if the pre-round-2 version is needed verbatim.
- Related: [[../bambuHLS/HANDOFF.md]] (general HAPI × Bambu compatibility,
  same device/clock conventions), `HAPI/examples/hls_fir/README.md`
  (baseline numbers everything here was compared against)
