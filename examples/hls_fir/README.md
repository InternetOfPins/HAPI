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
`env.AddCustomTarget`):

```sh
pio run -e hls -t synthesize-fir4               # fir4Top, compile-time coeffs
pio run -e hls -t synthesize-fir8               # fir8Top, compile-time coeffs
pio run -e hls -t synthesize-fir4-rtcoeff       # firRtTop, runtime coeffs
pio run -e hls -t synthesize-fir-lpf4           # firLpf4Top, Hamming LPF
pio run -e hls -t synthesize-fir-lpf8           # firLpf8Top, Hamming LPF
pio run -e hls -t synthesize-fir-lpf-cascade2   # firLpfCascade2Top, cascaded stages
```

Without `BAMBU_APPIMAGE` set, all six fail immediately with a clear
message naming the missing prerequisite. RTL and Bambu's own logs land in
`.hls_out_fir4/`, `.hls_out_fir8/`, `.hls_out_fir4_rtcoeff/`,
`.hls_out_fir_lpf4/`, `.hls_out_fir_lpf8/`, `.hls_out_fir_lpf_cascade2/`
respectively (gitignored).

## Results (verified, not estimated)

| | `fir4Top` (compile-time coeffs) | `fir8Top` (compile-time coeffs) | `firRtTop` (runtime coeffs) |
|---|---|---|---|
| Flip-flops | **32** | **85** | **267** |
| Registers | 1 | 4 | 21 (SE:12 + STD:9) |
| Multiplexers (2:1 equiv.) | 0 | 0 | 7 |
| `mult_expr_FU` | **0** | **0** | **2** |
| **Estimated number of DSPs** | **0** | **0** | **2** |
| Control steps | 4 | 4 | 8 |
| States / cycles | 2 / 2 | 2 / 2 | 6 / 6 |
| Estimated max frequency | 126.28 MHz | 124.35 MHz | 127.25 MHz |
| Minimum slack | 2.081 ns | 1.958 ns | 2.141 ns |
| Total estimated area | 7598 | 15302 | 5928 |

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
7598→15302 (2.01x — almost exactly linear). **Latency did not scale**:
both stay at 2 states / 2 cycles minimum-and-maximum — Bambu scheduled
every tap's shift-add network to complete combinationally within the same
2-cycle window regardless of tap count, so 8 taps cost roughly double the
silicon but not more time. Estimated max frequency stayed essentially flat
(126.28 → 124.35 MHz), consistent with a slightly longer combinational
path per cycle rather than added pipeline depth.

### Runtime coefficients: a real multiplier appears

`firRtTop`'s coefficients come from a `volatile int16_t coeffTable[4]` —
`volatile` forces a genuine memory read Bambu cannot constant-fold, the
same technique (real hardware input port, nothing to fold before
scheduling) `hls_smoke`'s `x` parameter and OneParse's `jsonBufTop`'s `n`
already relied on. Same 4-tap shape, same math (verified identical native
output to `fir4Top`, see above), and this time:

- **`mult_expr_FU: 2`, `Estimated number of DSPs: 2`** — real,
  DSP-mappable multiplier hardware, unlike either compile-time variant.
- Only **2** multiplier instances cover **4** logical tap multiplies —
  Bambu's own binding/allocation algorithm chose to time-share 2 physical
  multipliers across the 4 taps rather than instantiate 4, and added
  **7 muxes** (`MUX_GATE`) to route operands into the shared units. This
  is textbook HLS resource sharing, visible directly in the report.
- That sharing costs real latency: control steps go 4→8 and states/cycles
  go 2→6 (vs. either compile-time variant), and flip-flops jump to 267
  (register binding explicitly reports a *sub-optimal* result here — "21
  registers (LB:14)" — the extra scheduling complexity of shared
  multiplier access, not a free lunch).
- Estimated max frequency (127.25 MHz) and minimum slack (2.141 ns) stayed
  comparable to the compile-time variants — the critical path through one
  real multiplier is not, by itself, tighter than the shift-add chains
  above; the cost of this variant shows up entirely in cycle count and
  flip-flop area, not clock speed.
- Counter-intuitively, **total estimated area is lower** than `fir4Top`'s
  (5928 vs. 7598), despite the added multiplier hardware: `fir4Top`'s 4
  fully-unrolled shift-add networks (one per tap, nothing shared) cost
  more combinational area than 2 time-shared real multipliers plus their
  routing muxes. DSP count alone is not the whole resource picture —
  which is exactly why this variant is reported as three separate metrics
  (DSPs, flip-flops, area) rather than collapsed into one "cost" number.

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
| Estimated max frequency | 102.09 MHz | 102.09 MHz | 101.00 MHz |
| Minimum slack | 0.205 ns | 0.205 ns | 0.099 ns |
| Total estimated area | 7661 | 15328 | 15253 |

Same-question checklist as the [Results](#results-verified-not-estimated)
table above, answered with real numbers:

- **Does coefficient *value* change Bambu's resource report the way
  coefficient *source* does?** Not the way originally guessed. `firLpf4Top`
  lands close to `fir4Top` (32 FF vs 32, area 7661 vs 7598 — both
  Q8-Hamming and binomial 4-tap kernels cost almost the same). But
  `firLpf8Top` does **not** land close to `fir8Top`: 49 FF vs 85, area
  15328 vs 15302 (area is close; FF is not). Same topology, same control
  step/cycle count, genuinely different flip-flop count — the specific
  constants (1,10,41,76 vs 1,7,21,35) shift-add into different-sized
  hardware even though both strength-reduce to zero multipliers. Coefficient
  value *can* move the FF number; it just doesn't move it predictably.
- **Does `firLpfCascade2Top` cost roughly 2× a single `firLpf4Top`?** Total
  estimated area does: 15253 vs 7661 is **1.99×** — almost exactly double,
  the cleanest confirmation that Bambu built two genuinely independent
  filter sections, not one shared/merged design. Flip-flops did **not**
  double (32 vs 32, identical) — but flip-flop count is the wrong signal to
  read here: the RTL (`firLpfCascade2Top.v`) declares **8** separate
  `ARRAY_1D_STD_DISTRAM_NN_SDS` instances, i.e. 8 independent per-tap delay
  elements (matching `firLpf8Top`'s 8, as expected for 4+4 real taps) —
  Bambu binds each `Tap<>`'s one-sample delay register into a small
  distributed-RAM primitive, not a plain flip-flop, so "flip-flops" alone
  undercounts the real per-tap state cost. `ARRAY_1D_STD_DISTRAM_NN_SDS`
  is the reliable structural indicator; flip-flop count is not.
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
- **Minimum slack dropped** to 0.099 ns for the cascade (vs. 0.205 ns for
  either single stage) — the combinational path through two stages'
  worth of shift-add logic in the same clock period leaves noticeably
  less timing margin, the real cost of keeping latency flat at 2 cycles
  while doubling the combinational depth.

## Not done (if a real number is needed)

Same scope boundary as OneParse's `hls_smoke`:

- **Real P&R DSP-slice inference** (Vivado / Yosys+nextpnr-ecp5) — Bambu's
  `Estimated number of DSPs` and `mult_expr_FU` count characterize
  pre-placement functional units, not confirmed post-P&R DSP48/MULT18X18
  block mapping on a specific device family.
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
