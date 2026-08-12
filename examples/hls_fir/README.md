# hls_fir

Proof that HAPI's `Chain<>` composes **stateful** sections through a real
HLS backend, not just the stateless layers `hls_smoke` already covers. Each
tap of an N-tap FIR filter is one `Chain<>` layer, owning its own
single-sample delay register — composition depth literally *is* filter
order, the same relationship `hls_smoke` shows between its 5 `WrapWith<>`
layers and the folded adder.

It also answers a question `hls_smoke` and OneParse's JSON grammar never
needed to: does a *genuine* DSP-mappable multiplier get synthesized when
one is actually required? The answer is nuanced and verified both ways —
see [Results](#results-verified-not-estimated).

Beyond a single filter, it also checks whether `Chain<>` needs a new
primitive to compose *cascaded filter sections* (stage 2 processing stage
1's output, the textbook DSP "cascaded sections" shape) — see
[Cascaded stages](#cascaded-stages).

## Target device

Bambu is target-aware, not target-independent: functional-unit selection
(e.g. whether a multiply maps to a real DSP block or LUT/shift-add
fabric) and every area/frequency/slack number are characterized against a
specific device technology library — a run with no `--device-name`
produces numbers against Bambu's undocumented internal default, which
aren't citable against any real, ownable board.

All six targets below (and `extra_hls.py`'s `synthesize-*` custom
targets, which now always pass these flags) synthesize against:

```
--device-name=xc7a100t-1csg324-VVD --clock-period=10
```

`xc7a100t-1csg324-VVD` is the Xilinx Artix-7 on the Digilent Arty A7 /
Nexys A7 — widely owned, not a special-order part (confirmed as one of
Bambu's built-in named devices via its own `Available devices:` catalog
dump, not a hand-assembled device spec). 10ns targets 100MHz, in the
ballpark of OneParse's `jsonCharTop`/`jsonBufTop` results for rough
comparability.

**Confirming what the original numbers ran against:** checking
`extra_hls.py`'s history shows `fir4Top`/`fir8Top`/`firRtTop` were never
run with an explicit `--device-name`/`--clock-period` — those numbers
(as originally published) were against Bambu's undocumented default, not
a confirmed device. All three were re-run for this pass against the
explicit device above, alongside the three Hamming-LPF/cascade targets,
so every number in both Results tables below is now directly comparable.

**What changed and what didn't, re-running the original three targets
against a confirmed device instead of the default:** every *structural*
count — flip-flops, registers, DSP count, `mult_expr_FU` count, mux
count, control steps, states/cycles — came back **identical** to the
original default-device numbers. Only the *continuously-valued* metrics
that come directly from the device's characterized timing/area library
(estimated max frequency, minimum slack, total estimated area) shifted,
by single-digit percentages. In other words: every non-numeric claim in
this README (does a real multiplier appear, does resource sharing
happen, does latency scale with tap count, …) was already correct: only
the specific frequency/area/slack figures needed a confirmed device to
be citable, and one genuinely device-sensitive finding did turn up — see
[Cascaded stages](#cascaded-stages)'s corrected slack finding below.

## The targets

- **`fir4Top(int16_t x)`** (`hls/fir4_top.cpp`) — 4-tap FIR,
  `Chain<Tap<1>,Tap<3>,Tap<3>,Tap<1>>`. Coefficients are template NTTPs
  (compile-time constants).
- **`fir8Top(int16_t x)`** (`hls/fir8_top.cpp`) — 8-tap FIR,
  `Chain<Tap<1>,Tap<7>,Tap<21>,Tap<35>,Tap<35>,Tap<21>,Tap<7>,Tap<1>>`.
  Same `Tap<Coeff>` shape as the 4-tap, twice the layers — the
  resource-scaling half of this example.
- **`firRtTop(int16_t x)`** (`hls/fir4_rtcoeff_top.cpp`) — the same 4-tap
  shape, but each `Tap<Idx>` reads its coefficient from a `volatile`
  runtime table instead of using it as a template NTTP. Models a real,
  different DSP use case (a runtime-configurable/adaptive filter, e.g. a
  tunable EQ) where the coefficient genuinely cannot be known at synthesis
  time — the DSP-inference half of this example.
- **`firLpf4Top(int16_t x)`** (`hls/fir_lpf4_top.cpp`) — same 4-tap shape
  as `fir4Top`, but real Hamming-windowed-sinc low-pass coefficients
  (fc=1000Hz, fs=8000Hz, Q8 fixed-point, sum=256) instead of the binomial
  placeholder below — a filter a DSP audience recognizes on sight.
- **`firLpf8Top(int16_t x)`** (`hls/fir_lpf8_top.cpp`) — 8-tap version of
  the same Hamming-LPF design.
- **`firLpfCascade2Top(int16_t x)`** (`hls/fir_lpf_cascade2_top.cpp`) —
  two independent `firLpf4Top`-equivalent stages composed in series,
  stage 1's output feeding stage 2's input. See
  [Cascaded stages](#cascaded-stages) below — this is the actual
  composition-power demonstration; a single filter swap (the two targets
  above) doesn't show anything about `Chain<>` that `fir4Top`/`fir8Top`
  hadn't already shown via tap count.

The 4-tap/8-tap binomial kernels (`fir4Top`/`fir8Top`/`firRtTop`) are real
Pascal's-triangle smoothing-filter coefficients, not arbitrary numbers —
chosen because they're hand-checkable (see below), the same spirit as
`hls_smoke`'s hand-checkable `888`. The Hamming-LPF kernels
(`firLpf4Top`/`firLpf8Top`/`firLpfCascade2Top`) are a second, independently
real textbook design, added specifically to read as a recognizable filter
rather than an arbitrary integer sequence — both are legitimate, neither
supersedes the other; the binomial targets stay as they were.

All fixed-point: `int16_t` samples/coefficients, `int32_t` accumulator (no
floating point in this first pass — see [Not done](#not-done-if-a-real-number-is-needed)).

Each `hls/*.cpp` is an isolated, single-purpose translation unit (not part
of `src/`, so PlatformIO's native build doesn't compile them) — one
top-level global object per file, so the synthesized footprint reflects
only that entry point's real cost.

## Run it natively (regression check, all six)

```sh
pio run -e native
.pio/build/native/program
```

Every tap's delay register (`z`) starts at 0, so feeding an impulse
(`1, 0, 0, 0, ...`) into an N-tap FIR reads the filter's own coefficients
back out, one per sample, delayed by one call (the pipeline's one-sample
latency through the first tap) — hand-checkable the same way `hls_smoke`'s
`888` is:

```
fir4 impulse response (expect 0 1 3 3 1 0 0):
0 1 3 3 1 0 0
fir8 impulse response (expect 0 1 7 21 35 35 21 7 1 0 0):
0 1 7 21 35 35 21 7 1 0 0
fir4rt impulse response (expect 0 1 3 3 1 0 0):
0 1 3 3 1 0 0
firLpf4 impulse response (expect 0 10 118 118 10 0 0):
0 10 118 118 10 0 0
firLpf8 impulse response (expect 0 1 10 41 76 76 41 10 1 0 0):
0 1 10 41 76 76 41 10 1 0 0
firLpfCascade2 impulse response (expect 0 0 100 2360 16284 28048 16284 2360 100 0 0 0):
0 0 100 2360 16284 28048 16284 2360 100 0 0 0
```

`fir4rt` reproduces `fir4`'s output exactly — same coefficients, same
math, only the *source* of the coefficient (NTTP vs. runtime table)
differs, which is the entire point of that variant.

`firLpfCascade2`'s output is `firLpf4`'s coefficient list convolved with
itself (`conv([10,118,118,10], [10,118,118,10]) = [100,2360,16284,28048,
16284,2360,100]`, a standard, hand/`numpy.convolve`-checkable DSP fact),
delayed by 2 samples instead of `firLpf4`'s 1 — one sample of latency per
cascaded stage. See [Cascaded stages](#cascaded-stages) for why that's the
proof the composition is a real series cascade and not the tap-list
concatenation trap.

## Run it through Bambu HLS

Get the prebuilt AppImage (no Docker, no LLVM/GCC build needed):

```sh
curl -L -o bambu.AppImage https://release.bambuhls.eu/bambu-2024.10.AppImage
chmod +x bambu.AppImage
export BAMBU_APPIMAGE="$(pwd)/bambu.AppImage"
```

Bambu's frontend compiles internally as a 32-bit (`i386`) target, so it
needs 32-bit glibc headers most desktop installs don't have by default:

```sh
sudo apt install gcc-multilib g++-multilib   # or just libc6-dev-i386
```

> Before running that on a machine with a large pending-upgrade backlog,
> dry-run it first with `apt-get install -s gcc-multilib g++-multilib`
> (no root needed) — on a system that's behind on updates it can pull in
> far more than the two named packages, including a kernel update. Not
> expected on a clean/up-to-date system.

Then synthesize any of the six targets (wired via `extra_hls.py`,
`env.AddCustomTarget` — the explicit `--device-name`/`--clock-period`
from [Target device](#target-device) above are baked into every
`synthesize-*` target, nothing extra to pass):

```sh
pio run -e hls -t synthesize-fir4               # fir4Top, compile-time coeffs
pio run -e hls -t synthesize-fir8               # fir8Top, compile-time coeffs
pio run -e hls -t synthesize-fir4-rtcoeff       # firRtTop, runtime coeffs
pio run -e hls -t synthesize-fir-lpf4           # firLpf4Top, Hamming LPF
pio run -e hls -t synthesize-fir-lpf8           # firLpf8Top, Hamming LPF
pio run -e hls -t synthesize-fir-lpf-cascade2   # firLpfCascade2Top, cascaded stages
```

Plus two independent-config cross-checks (see
[Cross-tool/cross-config validation](#cross-toolcross-config-validation)):

```sh
pio run -e hls -t synthesize-fir4-gcc8               # ...and the other 5 -gcc8 targets
pio run -e hls -t synthesize-fir4-altdevice          # fir4Top against a Lattice ECP5
```

Without `BAMBU_APPIMAGE` set, all targets fail immediately with a clear
message naming the missing prerequisite. RTL and Bambu's own logs land in
`.hls_out_fir4/`, `.hls_out_fir8/`, `.hls_out_fir4_rtcoeff/`,
`.hls_out_fir_lpf4/`, `.hls_out_fir_lpf8/`, `.hls_out_fir_lpf_cascade2/`
(and each's `_gcc8`/`_altdevice`-suffixed counterpart) respectively
(gitignored).

## Results (verified, not estimated)

Against the confirmed device from [Target device](#target-device) above
(`xc7a100t-1csg324-VVD`, `--clock-period=10`):

| | `fir4Top` (compile-time coeffs) | `fir8Top` (compile-time coeffs) | `firRtTop` (runtime coeffs) |
|---|---|---|---|
| Flip-flops | **32** | **85** | **267** |
| Registers | 1 | 4 | 21 (SE:12 + STD:9) |
| Multiplexers (2:1 equiv.) | 0 | 0 | 7 |
| `mult_expr_FU` | **0** | **0** | **2** |
| **Estimated number of DSPs** | **0** | **0** | **2** |
| Control steps | 4 | 4 | 8 |
| States / cycles | 2 / 2 | 2 / 2 | 6 / 6 |
| Estimated max frequency | 123.80 MHz | 126.57 MHz | 126.63 MHz |
| Minimum slack | 1.923 ns | 2.099 ns | 2.103 ns |
| Total estimated area | 7590 | 15286 | 5921 |

### Compile-time coefficients: Bambu optimizes the multiplier away

`fir4Top`/`fir8Top`'s coefficients (1, 3, 3, 1 / 1, 7, 21, 35, 35, 21, 7, 1)
are template NTTPs — genuine compile-time constants. Bambu's own
scheduler recognized that multiplying a runtime value by a small constant
is cheaper as shift+add than as a real multiplier, and **strength-reduced
every tap's multiply into `lshift_expr_FU`/`rshift_expr_FU`/`plus_expr_FU`
— zero DSPs, zero `mult_expr_FU`, in both variants.** This is not a
composition limitation; it is Bambu doing exactly what a good HLS backend
should do with a compile-time-known coefficient, and it's the realistic
outcome for the common case of a fixed, non-adaptive filter design baked
into ROM.

**Resource scaling, 4-tap → 8-tap** (same optimization, twice the taps):
flip-flops 32→85 (2.66x), operations 33→72 (2.18x), estimated area
7590→15286 — a different metric from the flip-flop ratio, not a
conflicting measurement of the same one; both a ~2.66x FF scaling and a
separately-scaling area number can be true at once, and both are
confirmed again against this pass's explicit device. **Latency did not
scale**: both stay at 2 states / 2 cycles minimum-and-maximum — Bambu
scheduled every tap's shift-add network to complete combinationally
within the same 2-cycle window regardless of tap count, so 8 taps cost
roughly double the silicon but not more time. Estimated max frequency
(123.80 → 126.57 MHz) and minimum slack (1.923 → 2.099 ns) both moved by
single-digit percentages — noise-level, not a trend worth reading into
tap count on its own.

### Runtime coefficients: a real multiplier appears

`firRtTop`'s coefficients come from a `volatile int16_t coeffTable[4]` —
`volatile` forces a genuine memory read Bambu cannot constant-fold, the
same technique (real hardware input port, nothing to fold before
scheduling) `hls_smoke`'s `x` parameter and OneParse's `jsonBufTop`'s `n`
already relied on. Same 4-tap shape, same math (verified identical native
output to `fir4Top`, see above), and this time:

- **`mult_expr_FU: 2`, `Estimated number of DSPs: 2`** — real,
  DSP-mappable multiplier hardware, unlike either compile-time variant.
  Confirmed against a real Artix-7 device library, not an unconfirmed
  default — this device genuinely has DSP48 blocks Bambu could target,
  so the inference is meaningful, not an artifact of a generic/default
  technology library that might not model hard multiplier blocks at all.
- Only **2** multiplier instances cover **4** logical tap multiplies —
  Bambu's own binding/allocation algorithm chose to time-share 2 physical
  multipliers across the 4 taps rather than instantiate 4, and added
  **7 muxes** (`MUX_GATE`) to route operands into the shared units. This
  is textbook HLS resource sharing, visible directly in the report.
- That sharing costs real latency: control steps go 4→8 and states/cycles
  go 2→6 (vs. either compile-time variant), and flip-flops jump to 267
  (register binding explicitly reports a *sub-optimal* result here — "21
  registers (LB:14)", the same SE:12+STD:9 breakdown confirmed again
  against the explicit device — the extra scheduling complexity of shared
  multiplier access, not a free lunch).
- Estimated max frequency (126.63 MHz) and minimum slack (2.103 ns) stayed
  comparable to the compile-time variants — the critical path through one
  real multiplier is not, by itself, tighter than the shift-add chains
  above; the cost of this variant shows up entirely in cycle count and
  flip-flop area, not clock speed.
- Counter-intuitively, **total estimated area is lower** than `fir4Top`'s
  (5921 vs. 7590), despite the added multiplier hardware: `fir4Top`'s 4
  fully-unrolled shift-add networks (one per tap, nothing shared) cost
  more combinational area than 2 time-shared real multipliers plus their
  routing muxes. DSP count alone is not the whole resource picture —
  which is exactly why this variant is reported as three separate metrics
  (DSPs, flip-flops, area) rather than collapsed into one "cost" number.
- **All of the above — DSP count, mux count, control steps, register
  breakdown — matched exactly** between the original (unconfirmed-device)
  run and this pass's confirmed-device re-run; only the frequency/slack/
  area figures moved, by single-digit percentages. The structural finding
  ("a real multiplier appears when coefficients are genuinely runtime")
  was never in question — this pass makes the specific numbers citable
  against a real, ownable device.

### Cross-tool/cross-config validation

Bambu is currently the only HLS backend actually run against these designs
— a Bambu-specific quirk could in principle masquerade as a HAPI property
(or vice versa). Two independent Bambu configs were run as a first
cross-check. Three more independent tools were investigated:

| Tool | Status |
|---|---|
| Bambu (clang16 frontend) | **Done** — primary Results table above |
| Bambu (GCC8 frontend) | **Done** — rejects HAPI on all six targets, see below |
| Bambu (Lattice ECP5 device) | **Done**, `fir4Top` only — see below |
| Vitis HLS | **Not run** — integration scaffolding ready (`extra_hls_vitis.py`, `vitis/run_hls.tcl`, `[env:hls-vitis]`, all six targets); blocked on Xilinx account + Vitis Unified Installer, an interactive step not done in this pass |
| Intel HLS Compiler | **Not run** — integration scaffolding ready (`extra_hls_intel.py`, `[env:hls-intel]`, all six targets); blocked on Intel/Altera account + Quartus Prime Lite download, an interactive step not done in this pass; will also target a different (Altera/Intel) device family, not `xc7a100t-1csg324-VVD` |
| LegUp | **Ruled out** — free academic version is a frozen, single-commit, pre-C++17, VM-only 2015-era snapshot under a non-commercial license; actively-maintained descendant (SmartHLS) is closed/commercial. Full reasoning: `HAPI/.RnD/legupHLS/HANDOFF.md` |

To run the Vitis HLS / Intel HLS Compiler targets once installed:

```sh
export VITIS_HLS=/path/to/vitis_hls   # after Xilinx account + Vitis HLS install
pio run -e hls-vitis -t synthesize-fir4-vitis   # ...and the other 5

# after Quartus Prime Lite + Intel HLS Compiler install, with i++ on PATH
pio run -e hls-intel -t synthesize-fir4-intel   # ...and the other 5
```

- **GCC8 frontend (`--compiler=I386_GCC8`): rejected on all six targets,
  not a HAPI bug.** Every one of `synthesize-fir4-gcc8` through
  `synthesize-fir-lpf-cascade2-gcc8` fails identically, at parse time —
  `Unrecognized keyword ... bound_template_template_parm` / `Parse error` —
  before scheduling is ever reached. Bambu's GCC8-based tree parser doesn't
  recognize the AST node for a template-template-parameter binding that
  `Chain<>`'s recursive composition produces; the bundled clang16 frontend
  accepts the same construct cleanly across all six designs. `I386_CLANG16`
  is the only viable frontend for this codebase — see
  `HAPI/.RnD/bambuHLS/HANDOFF.md` finding #4 for the full writeup.
- **Lattice ECP5 device (`LFE5U85F8BG756C`), `fir4Top` only: DSP inference
  and latency are device-independent, flip-flop/area/frequency are not.**

  | Metric | Bambu / clang16, Artix-7 (primary) | Bambu / clang16, Lattice ECP5 |
  |---|---|---|
  | Flip-flops | **32** | **51** |
  | `mult_expr_FU` | **0** | **0** |
  | **Estimated number of DSPs** | **0** | **0** |
  | Control steps | 4 | 4 |
  | States / cycles | 2 / 2 | 2 / 2 |
  | Estimated max frequency | 123.80 MHz | 102.78 MHz |
  | Minimum slack | 1.923 ns | 0.270 ns |
  | Total estimated area | 7590 | 6101 |

  DSP count (0), `mult_expr_FU` count (0), and control-step/cycle count all
  matched exactly against a second, non-Xilinx vendor's device — the
  strongest form of evidence that "Bambu strength-reduces the compile-time
  multiply away" (see above) is a real property of the design, not a
  Bambu/Artix-7 artifact. Flip-flop count (32 → 51), total area (7590 →
  6101), and timing figures did **not** match — expected, since each
  device's technology library binds the same logical registers and
  timing differently; these numbers were never claimed to be portable
  across devices (see [Target device](#target-device) above), and this run
  confirms that boundary rather than overturning it.

## Cascaded stages

`firLpf4Top`/`firLpf8Top` swap in real Hamming-windowed-sinc coefficients
but don't demonstrate anything about `Chain<>` that `fir4Top`/`fir8Top`
hadn't already shown via tap count. `firLpfCascade2Top`
(`hls/fir_lpf_cascade2_top.cpp`) is the actual composition-power
demonstration: two independent `firLpf4Top`-equivalent stages in series,
stage 1's fully-summed output feeding stage 2 as its input sample stream —
the textbook "cascaded sections" shape from filter design, native
vocabulary to any DSP audience.

### The open question this answers: does `Chain<>` need a new primitive?

No — and the reason is structural, not a missing feature. `Chain<>`'s
composition algebra threads *one running accumulator* across taps that all
see progressively delayed copies of the *same* input sample stream — that
is exactly what a single filter's internal topology needs (which is what
`Tap<>` already gives you), but it is a different shape from a cascade of
independent filter *sections*, where stage 2 must operate on stage 1's
fully-summed *output*, not on a further-delayed copy of the raw input.

Concretely, concatenating the two stages' tap lists via `Chain<>::App`/
`Ins` —

```cpp
Chain<Tap<10>,Tap<118>,Tap<118>,Tap<10>, Tap<10>,Tap<118>,Tap<118>,Tap<10>>
```

— does **not** produce a cascade. It silently produces a *different*,
single 8-tap FIR whose coefficients are just the two lists side by side.
The two impulse responses make the difference concrete:

```
naive Chain<> concatenation (wrong):  0 10 118 118 10 10 118 118 10 0 0
true cascade, firLpfCascade2Top:      0 0 100 2360 16284 28048 16284 2360 100 0 0 0
```

The first is just `firLpf4Top`'s coefficients read back twice — no actual
filtering interaction between the two "stages" (they were never really two
stages, just eight taps sharing one accumulator). The second is the
convolution of `firLpf4Top`'s impulse response with itself, exactly what a
real two-section cascade produces, delayed by 2 samples (one per stage's
own one-sample latency) instead of 1. This is a genuine correctness trap
for anyone reaching for `Chain<>` to build a cascade by instinct — tap-list
concatenation is the right move *within* one filter (that's the whole
point of `fir8Top` vs `fir4Top`), and the wrong move *across* filters.

### The actual composition: no new primitive needed

A "stage" in a cascade is not a tap — it's an already-closed top-level
transform, an `APIOf<Item, Chain<Tap<...>...>>` instance that exposes one
black-box `mac(x, 0) -> y` call. Composing two closed transforms in series
is exactly what composing any two closed C++ function calls already is:

```cpp
using Stage = APIOf<Item, Chain<Tap<10>, Tap<118>, Tap<118>, Tap<10>>>;
Stage stage1, stage2;

int32_t firLpfCascade2Top(int16_t x) {
  int32_t y1 = stage1.mac(x, 0);
  return stage2.mac(static_cast<int16_t>(y1), 0);   // stage 1's output IS stage 2's input
}
```

No new `Chain<>` machinery, no core library change — `Chain<>` stays scoped
to what it's actually for (composing taps that share one filter's running
accumulator), and stage-to-stage cascading is answered by HAPI's existing
guarantee that a composed chain collapses into an ordinary, callable
object. This is the honest finding: not a gap to close, but a boundary
worth documenting so the next person reaching for `Chain<>::Ins` to build
a cascade doesn't fall into the concatenation trap above.

One real limitation, scoped out deliberately for this fixed-point-only
first pass (same boundary as the rest of this README): `firLpfCascade2Top`
does not renormalize between stages. Each section's coefficients sum to
256 (Q8, unity DC gain once divided by 256), so a real audio-scale design
would right-shift (and saturate) `y1` before feeding it to stage 2 to keep
the signal in `int16_t` range as cascaded gain compounds. That shift is
skipped here because the native regression check above uses an impulse of
amplitude 1 — a `>>8` between stages would truncate every intermediate
value to zero and defeat the hand-checkable test. Floating point and
inter-stage normalization/saturation are both reasonable follow-ups, not
blockers for this pass.

### Bambu synthesis results (verified, not estimated)

Against the same confirmed device as the main
[Results](#results-verified-not-estimated) table
(`xc7a100t-1csg324-VVD`, `--clock-period=10`):

| | `firLpf4Top` | `firLpf8Top` | `firLpfCascade2Top` |
|---|---|---|---|
| Flip-flops | **32** | **49** | **32** |
| Registers | 1 | 3 | 1 |
| Distributed-RAM delay elements (`ARRAY_1D_STD_DISTRAM_NN_SDS`) | 4 | 8 | 8 |
| Multiplexers (2:1 equiv.) | 0 | 0 | 0 |
| `mult_expr_FU` | **0** | **0** | **0** |
| **Estimated number of DSPs** | **0** | **0** | **0** |
| Control steps | 4 | 4 | 4 |
| States / cycles | 2 / 2 | 2 / 2 | 2 / 2 |
| Estimated max frequency | 103.33 MHz | 103.33 MHz | 103.33 MHz |
| Minimum slack | 0.322 ns | 0.322 ns | 0.322 ns |
| Total estimated area | 7653 | 15312 | 15237 |

Same-question checklist as the [Results](#results-verified-not-estimated)
table above, answered with real numbers:

- **Does coefficient *value* change Bambu's resource report the way
  coefficient *source* does?** Not the way originally guessed. `firLpf4Top`
  lands close to `fir4Top` (32 FF vs 32, area 7653 vs 7590 — both
  Q8-Hamming and binomial 4-tap kernels cost almost the same). But
  `firLpf8Top` does **not** land close to `fir8Top`: 49 FF vs 85, area
  15312 vs 15286 (area is close; FF is not). Same topology, same control
  step/cycle count, genuinely different flip-flop count — the specific
  constants (1,10,41,76 vs 1,7,21,35) shift-add into different-sized
  hardware even though both strength-reduce to zero multipliers. Coefficient
  value *can* move the FF number; it just doesn't move it predictably.
  **Confirmed a second time against the explicit device** — this isn't a
  default-device artifact, the mismatch survives re-synthesis against
  real Artix-7 characterization.
- **Does `firLpfCascade2Top` cost roughly 2× a single `firLpf4Top`?** Total
  estimated area does: 15237 vs 7653 is **~1.99×** — almost exactly
  double, the cleanest confirmation that Bambu built two genuinely
  independent filter sections, not one shared/merged design. Flip-flops
  did **not** double (32 vs 32, identical) — but flip-flop count is the
  wrong signal to read here: the RTL (`firLpfCascade2Top.v`) declares
  **8** separate `ARRAY_1D_STD_DISTRAM_NN_SDS` instances, i.e. 8
  independent per-tap delay elements (matching `firLpf8Top`'s 8, as
  expected for 4+4 real taps) — Bambu binds each `Tap<>`'s one-sample
  delay register into a small distributed-RAM primitive, not a plain
  flip-flop, so "flip-flops" alone undercounts the real per-tap state
  cost. `ARRAY_1D_STD_DISTRAM_NN_SDS` is the reliable structural
  indicator; flip-flop count is not.
- **Does resource-sharing extend across cascaded stages the way it shares
  within one stage** (2 physical multipliers covering 4 logical taps in
  `firRtTop`)? Not applicable here — both stages are compile-time-
  coefficient, so both strength-reduce to shift+add with zero multipliers
  to share in the first place. Answering the sharing question for real
  would need a *runtime-coefficient* cascade (two `firRtTop`-shaped
  stages), not attempted in this pass.
- **Control steps / cycles stayed flat at 2/2** for all three, same as
  every compile-time-coefficient target in the main
  [Results](#results-verified-not-estimated) table — cascading two stages
  cost real area (≈2×) but not extra latency; Bambu scheduled the whole
  two-stage combinational path to complete within the same 2-cycle window
  as a single stage.
- **Correction from an earlier, unconfirmed-device pass of this same
  table:** a prior run (against Bambu's undocumented default device, not
  the explicit Artix-7 device this table uses) showed minimum slack
  dropping for the cascade (0.099 ns) versus either single stage
  (0.205 ns), read at the time as "the real cost of doubling combinational
  depth while keeping latency flat." **That finding does not survive
  re-synthesis against a confirmed device** — here, `firLpf4Top`,
  `firLpf8Top`, and `firLpfCascade2Top` all report essentially identical
  minimum slack (0.322 ns, differing only in the 8th significant digit)
  and identical estimated max frequency (103.33 MHz). Whatever produced
  the apparent slack drop under the default/unconfirmed device did not
  reproduce under real device characterization — a concrete example of
  why device-unconfirmed timing numbers aren't safe to build a narrative
  on, and exactly the risk [Target device](#target-device) above exists
  to close off.

## Not done (if a real number is needed)

Same scope boundary as OneParse's `hls_smoke`:

- **Real P&R DSP-slice inference** (Vivado / Yosys+nextpnr-ecp5) — now
  characterized against a real, named device (`xc7a100t-1csg324-VVD`,
  see [Target device](#target-device)) rather than an unconfirmed
  default, but Bambu's `Estimated number of DSPs` and `mult_expr_FU`
  count still characterize pre-placement functional units, not confirmed
  post-P&R DSP48/MULT18X18 block mapping — that step still needs Vivado
  or Yosys+nextpnr-ecp5 and hasn't been run.
- **RTL simulation for real cycle-accurate throughput** (Verilator /
  Icarus) — the control-step/state counts above are Bambu's static
  schedule, not a simulated, testbench-driven measurement.
- **Floating point** — fixed-point only in this pass; a reasonable
  follow-up once this pattern's proven, not a blocker for v1.

## Stretch goal, not started

`firLpfCascade2Top` above cascades two FIR *sections*, but each section is
still built from plain FIR taps — single-direction shift registers. A
cascaded **biquad** chain (classic audio-EQ shape, several biquad sections
in series) would test a genuinely different composition shape: each
biquad section carries its own multi-tap delay-line *state* across two
directions (feedforward and feedback/IIR), unlike a plain FIR tap. Worth
doing specifically because it's a different test of the composition
model, not just a bigger or cascaded FIR — not attempted here.
