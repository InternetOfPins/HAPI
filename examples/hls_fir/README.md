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

## The three targets

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

Both 4-tap and 8-tap kernels are real binomial (Pascal's-triangle)
smoothing-filter coefficients, not arbitrary numbers — a legitimate,
textbook FIR design, chosen because they're also hand-checkable (see
below), the same spirit as `hls_smoke`'s hand-checkable `888`.

All fixed-point: `int16_t` samples/coefficients, `int32_t` accumulator (no
floating point in this first pass — see [Not done](#not-done-if-a-real-number-is-needed)).

Each `hls/*.cpp` is an isolated, single-purpose translation unit (not part
of `src/`, so PlatformIO's native build doesn't compile them) — one
top-level global object per file, so the synthesized footprint reflects
only that entry point's real cost.

## Run it natively (regression check, all three)

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
```

`fir4rt` reproduces `fir4`'s output exactly — same coefficients, same
math, only the *source* of the coefficient (NTTP vs. runtime table)
differs, which is the entire point of that variant.

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

Then synthesize any of the three targets (wired via `extra_hls.py`,
`env.AddCustomTarget`):

```sh
pio run -e hls -t synthesize-fir4          # fir4Top, compile-time coeffs
pio run -e hls -t synthesize-fir8          # fir8Top, compile-time coeffs
pio run -e hls -t synthesize-fir4-rtcoeff  # firRtTop, runtime coeffs
```

Without `BAMBU_APPIMAGE` set, all three fail immediately with a clear
message naming the missing prerequisite. RTL and Bambu's own logs land in
`.hls_out_fir4/`, `.hls_out_fir8/`, `.hls_out_fir4_rtcoeff/` respectively
(gitignored).

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

A cascaded biquad chain (classic audio-EQ shape, several biquad sections
in series) would test a genuinely different composition shape than this
FIR: each biquad section carries its own multi-tap delay-line *state*
across two directions (feedforward and feedback), unlike a plain FIR
tap's single-direction shift register. Worth doing specifically because
it's a different test of the composition model, not just a bigger filter
— not attempted here.
