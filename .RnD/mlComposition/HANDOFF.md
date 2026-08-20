# HANDOFF: HAPI + ML

Written by a sandboxed agent with AVR cross-compilation only (no ESP32/STM32,
no real sensor hardware), for a local agent that has more. Same rule as the
other `.RnD/*/HANDOFF.md` files: **verified** = actually cloned/compiled/
measured this session, not estimated. **TODO** = needs your toolchain/hardware
or your explicit go-ahead.

## Origin

The open question from the parent conversation: `HAPI/docs/INDUSTRY.md`
already has an "Edge AI & TinyML" section with a deliberately conservative,
currently-unverified claim — HAPI structures the pipeline *around* inference
(`Sensor → Calibration → Filtering → Feature Extraction → Quantisation →
Inference Input`), explicitly **not** the inference math itself ("HAPI does
not accelerate neural-network kernels or execution engines"). Two branches
were on the table:

1. Verify the documented claim as written, stay inside its stated scope.
2. Test whether a `Dense`+`Activation` layer stack — structurally close to
   `oneHLS`'s existing `Fir`/`Biquad` MAC chains — composes at zero overhead
   too, which would *revise* the "does not accelerate NN kernels" line, not
   just support it.

Rather than hand you a plan for one and a question mark for the other, I
built and measured both. Real numbers below for each; the decision on branch
2 is still yours — see the end.

## Experiment 1 — the documented claim, verified

`stage1_ml_pipeline.cpp` composes `Sensor → Calibration → Filtering → Feature
Extraction → Quantisation → Inference Input` on real `atmega328p`, reusing
existing IOP components everywhere the pipeline stage already has one:

| Stage | Component | Status |
|---|---|---|
| Sensor | `oneIO::sensor::AHT<I2c>` | real, reused as-is (same one Stage 1 of the IoT handoff verified) |
| Calibration | `Calibrate<OffsetX10>` | **new** — no existing IOP component for this, kept to one small chain-splice-able `Part` |
| Filtering | `oneHLS::Biquad<int16_t,int32_t,...>` | real, reused as-is — plain member, same idiom as the IoT demo's `Pid` |
| Feature Extraction | trivial passthrough | not a real gap to fill yet — genuine feature engineering is domain-specific, out of scope for a structural demo |
| Quantisation | hand-written, same discipline as `oneHLS`'s `rawCoeff`/`RawBitsCtor` | new code, established pattern |
| Inference Input | explicit stub (`inferenceInput(int8_t)`, `// TODO: real model call`) | marks the documented scope boundary on purpose |

One composition wrinkle worth carrying forward: `Calibrate` has to be listed
**before** `Sensor` in the `Chain<>` (`Chain<Calibrate<-3>, Sensor>`, not the
other way round) so it can reach `Sensor`'s `tempC10()` via `O::` — first-
listed is outermost/most-derived, same rule the blockchain-kernel HANDOFF
already documented for cross-component dependencies.

Compiled and linked clean, real `atmega328p`:

```
Program:  986 bytes (3.0% of 32K)
Data:      14 bytes (0.7% of 2K)
indirect calls (callx/icall): 0
```

This closes the documented claim for real, on the exact scope INDUSTRY.md
already states. TODO for you: ESP32/STM32 legs, and wiring `Feature
Extraction` to something less trivial once there's a real second sensor
signal to derive a feature from (accel + temp, once `MPU6050` stops being a
stub — see the IoT handoff's open decision on that).

## Experiment 2 — pushing past the documented line (proof of concept)

`stage2_inference_poc.cpp` is explicitly **not** meant to land anywhere yet.
It tests whether a fixed-shape `Dense → ReLU → Dense` stack — 3 inputs, 2
hidden units, 1 output, weights as compile-time NTTPs, hand-picked not
trained — composes the same way `Fir<>`'s tap chain does:

```cpp
using Net = Chain<Dense3to2<64,-32,16, -48,80,24>, Relu2,
                  Dense2to1<96,-40>>::Part<NetTerminal>;
```

Deliberately **not** a generic N-input/M-output `Dense<>` template — a fixed
3→2→1 shape only, same "avoid arbitrary shape/framework scope creep" rule
the blockchain and IoT experiments both used.

**First cut, inputs as literals** (`Net::forward(120, -15, 8)`): the compiler
constant-folded the entire 3-layer network — weights *and* inputs both known
at compile time — down to one `ldi r24, 0x0C` (12) in `main`. Real result,
but not the representative one; a live sensor value isn't a compile-time
constant.

**Corrected — weights as NTTPs (realistic: fixed after deployment), inputs
`volatile` (realistic: live data)**, same distinction INDUSTRY.md's own FIR
section already draws for filter coefficients, applied here to inference
input:

```
Program:  380 bytes (1.2% of 32K)
Data:       8 bytes (0.4% of 2K)
```

Disassembly of `main`: every layer — both `Dense`s and the `ReLU` — inlined
into one flat, straight-line instruction sequence. Not just zero *indirect*
calls; zero calls of *any* kind, direct or indirect. No per-layer dispatch,
no op loop, no interpreter — the entire forward pass is one function, which
is the sharper, more specific version of "no runtime graph interpreter" than
a plain indirect-call count gets you.

**Optional-elimination, re-tested for this exact composition shape** (not
just cited from the older GFX/blockchain results): a `DropoutNoop` layer
(pure passthrough — real dropout is training-only, a no-op at inference)
spliced into the same chain:

```cpp
using NetWithDropout = Chain<Dense3to2<...>, Relu2, DropoutNoop,
                             Dense2to1<...>>::Part<NetTerminal>;
```

`.text` is **byte-identical** with and without it. Confirmed by `cmp`, not
assumed by analogy.

### What this means, and what it doesn't

This is real evidence that a HAPI-composed inference stack can go past
"structural pipeline only" — but it's a 3-neuron toy with untrained,
hand-picked weights, fixed-point arithmetic via ad hoc `>>8` shifts (not a
principled quantization scheme), one sample per call (no throughput number),
and no comparison against what a runtime interpreter (TFLite Micro, etc.)
actually costs per op — that would need building one for real, which wasn't
attempted here. The evidence supports revising the INDUSTRY.md line; it
doesn't yet earn a specific claim about how it compares to any named
alternative, and it isn't wired to a real sensor or a real trained model.
Both of those, and whether to pursue this branch further at all, are your
call — I'd lean toward yes based on the numbers above, but "I'd lean toward"
is as far as this goes without you saying so.

## If you say go on Experiment 2 — staged plan

```
this PoC (done, above)
      ↓
wire to real sensor data — feed Experiment 1's pipeline output into this
net's input, closing both branches into one demo instead of two
      ↓
real trained weights — training itself stays entirely outside HAPI (wrong
domain, same reasoning as excluding backprop/autodiff from scope); train
offline (plain Python/numpy is enough for a 3-neuron net), bake the result
as NTTPs, same as this PoC's hand-picked ones
      ↓
embedded measurement — same AVR/ESP32/STM32 targets already set up for the
IoT demo, vs. a hand-written baseline doing the identical arithmetic
      ↓
OneHLS kernel — Dense-as-MAC-chain through Bambu/Vitis. Lower-friction than
it sounds: the whole RawBitsCtor/ac_fixed/ap_fixed-agnostic pattern OneHLS
already has for Fir/Biquad/Pid/ComplexMac transfers close to directly
      ↓
INDUSTRY.md update — only after the above holds up, with real citations,
same discipline used for the CAN/MQTT gap in the IoT handoff
```

Stop as soon as the composition thesis is convincingly shown, same rule
Rui's own blockchain framing used — not when the network gets interesting.

## Where this lives

`.RnD/mlComposition/` inside the `HAPI` checkout, matching
`.RnD/blockchainKernelHLS/` and `.RnD/iotComposition/`. Experiment 1
promotes to `examples/` once it has ESP32/STM32 legs and a PlatformIO
wrapper, same graduation path as `hls_blockchain_kernel`. Experiment 2 stays
in `.RnD/` — proof of concept, not proposed for promotion — until you decide
whether it's a real direction; if it is, `Calibrate` and whatever `Dense`/
`Relu` become are the individual pieces that would eventually graduate into
`OneIO`/`OneHLS` proper, not the demo device itself.

## Round 2 — reviewer feedback, addressed with real evidence

A second external pass on Round 1 raised four points genuinely not covered
above (ArgMax/decision, three distinct meanings of "optional," A/B/C
structural variants, weighted-sum as its own reusable primitive) and three
that restated what was already verified (zero overhead, heterogeneous
origins, cross-domain MCU/FPGA). This round addresses the four real ones —
built and measured, not just planned.

### `Neuron<Accum, W...>` — the weighted-sum primitive, factored out

`stage2_inference_poc.cpp`'s `Dense3to2` hand-inlined `W00*x0+W01*x1+W02*x2`
directly in its body. That's not a fair test of "are ML ops good HAPI
components" — it's one hand-fused example. Factored the weighted sum out as
its own reusable primitive instead, recursive over the weight/input packs
one term at a time — the `Tap<>` analog for Dense, same way `Fir<>`'s `Tap`
is reusable across arbitrary tap counts:

```cpp
template<typename Accum, Accum... Ws> struct Neuron;
template<typename Accum> struct Neuron<Accum> { static constexpr Accum sum() { return 0; } };
template<typename Accum, Accum W0, Accum... Ws>
struct Neuron<Accum, W0, Ws...> {
  template<typename X0, typename... Xs>
  static Accum sum(X0 x0, Xs... xs) { return Accum(W0*x0) + Neuron<Accum,Ws...>::sum(xs...); }
};
```

`Dense3to2`/`Dense2to3` are now built *from* this, not hand-written per
layer.

### A/B/C structural variants — correctness, not just footprint

Built three variants sharing the same components, differing only in the
`Chain<>` list — exactly the reviewer's suggested test:

```cpp
using NetA = Chain<Normalize3<...>, Dense3to2<...>, Relu2, Dense2to3<...>, ArgMax3>::Part<T>; // full
using NetB = Chain<Normalize3<...>, Dense3to2<...>,       Dense2to3<...>, ArgMax3>::Part<T>; // no ReLU
using NetC = Chain<                 Dense3to2<...>, Relu2, Dense2to3<...>, ArgMax3>::Part<T>; // no Normalize
```

`ArgMax3` is new — the decision stage the reviewer correctly flagged as
missing (ties the pipeline to an actual decision instead of stopping at a
raw number). Each variant's output was compared against an independent
"boring baseline" — plain C++, no `Chain<>`, no `Neuron<>`, same arithmetic
written out by hand — across 5 test inputs, same discipline Rui's other
experiments use a hand-written baseline for.

**First run found a real bug.** `Neuron<int16_t, W...>` — accumulator same
width as the weights/inputs — silently produced a wrong `ArgMax` class for
input `(300,-300,0)` on all three variants (composed=1, boring=0). Root
cause: summing several `int16_t` products in an `int16_t` accumulator
overflows; the boring baseline didn't have this bug only because C++
promotes `int16_t*int16_t` to `int` before narrowing, which happens to
paper over it. This is exactly the accumulator-width problem `oneHLS`
already solved for `Fir`/`Biquad`/`Pid` (`Sample` narrow, `Accum` wide,
e.g. `Pid<int16_t,int32_t,...>`) — the first cut of `Neuron<>` just didn't
apply that existing convention. Fixed by widening: `Neuron<int32_t, W...>`
sums in `int32_t`, narrows to `int16_t` only at the `>>8` step. Re-ran:

```
in( 120, -15,   8)  A: composed=0 boring=0 OK | B: composed=0 boring=0 OK | C: composed=0 boring=0 OK
in(   0,   0,   0)  A: composed=0 boring=0 OK | B: composed=0 boring=0 OK | C: composed=0 boring=0 OK
in( -50, 200,  30)  A: composed=1 boring=1 OK | B: composed=2 boring=2 OK | C: composed=1 boring=1 OK
in(  10,  10,  10)  A: composed=0 boring=0 OK | B: composed=0 boring=0 OK | C: composed=0 boring=0 OK
in( 300,-300,   0)  A: composed=0 boring=0 OK | B: composed=0 boring=0 OK | C: composed=0 boring=0 OK

ALL VARIANTS MATCH BORING BASELINE
```

This is the actual point of the reviewer's push: footprint being small and
call-free says nothing about whether the composition computes what the
selected components should produce. It didn't, the first time.

**Real AVR numbers for the corrected variants:**

| Variant | Flash | RAM | Indirect calls | Direct calls |
|---|---|---|---|---|
| A (full) | 646 B | 7 B | 0 | 13 (all `__mulhisi3`/`__umulhisi3`/`__usmulhisi3` — libgcc 16×32 multiply helpers) |
| B (no ReLU) | 682 B | 7 B | 0 | 15 (same helpers, +1 variant) |
| C (no Normalize) | 632 B | 7 B | 0 | 13 |

Worth being precise about these "calls": zero are indirect (`icall`/`callx`)
— the composition-dispatch thesis still holds exactly as before. The direct
calls are the 8-bit AVR core's compiler-inserted helpers for 16×16→32-bit
multiplication, an unavoidable cost of the width fix above, and completely
orthogonal to HAPI's composition mechanism — not something the original
(buggy, narrower) version paid, because it was computing the wrong answer
cheaply rather than the right answer at its real cost.

### Optionality — the third case, demonstrated

Round 1 verified one of the three cases the reviewer named (no-op → full
elimination, `DropoutNoop`, byte-identical `.text`). The "required but
missing → compile failure" case wasn't shown. Reused Experiment 1's real
`Calibrate`/`Sensor` pair rather than inventing a new example for this:

```cpp
using BrokenPipeline = Chain<Calibrate<-3>>::Part<EmptyTerminal>; // no Sensor
BrokenPipeline::calibratedTempC10();
```

Real compiler output, anchored at the actual call site (not buried in
library internals — the "no-inner-static-assert" property holding without
any special effort here):

```
error: 'tempC10' is not a member of 'EmptyTerminal'
```

Third case (default implementation stays present and does real work) is
implicit in A vs. C above — `Normalize3` is present and actually shifts the
decision boundary in A/B, absent in C — not re-demonstrated separately.

TODO, not done here: the same failure using HAPI's formal `rules()`/
`Requires<>` mechanism (already used in the blockchain-kernel work) instead
of a plain missing-member error, for a more actionable diagnostic than
"is not a member of."

### Does this change the recommendation on Experiment 2?

If anything, strengthens it. The point of building the correctness harness
was to find out whether it would catch something real — it did, on the
first run, and the fix was to apply a convention the ecosystem already has
rather than invent a new one. That's the methodology working as intended,
not a reason for less confidence in the branch. Still your call whether to
pursue it further — see Experiment 2's original caveats above, all of which
still apply (untrained weights, fixed shape, AVR only).

## Files in this handoff

- `HANDOFF.md` — this file.
- `stage1_ml_pipeline.cpp` — Experiment 1, compiles clean, numbers above.
- `stage2_inference_poc.cpp` — Experiment 2 PoC (Round 1), both variants
  (`WITH_DROPOUT` toggled via `-DWITH_DROPOUT`). Explicitly not proposed
  for promotion as-is.
- `variants.cpp` — Round 2's A/B/C correctness harness (native build,
  `Neuron<>`, `ArgMax3`, boring-baseline comparison, the accumulator bug
  and its fix, all in one file).
- `variants_avr.cpp` — same three variants, AVR footprint build
  (`-DVARIANT=0/1/2` for A/B/C).
- `missing_api.cpp` — the required-but-missing compile-failure demo.
