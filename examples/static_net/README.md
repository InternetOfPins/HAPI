# static_net

Static networks: dry, typed descriptions of dataflow nets.

A **net** is a typelist of parts, wired by index, by id or by a query. It carries no data of its own; the only runtime state is the input view it is evaluated on.
Because the whole structure is a type, the compiler resolves it at compile time: the forms below compile to the same disassembly as the hand-written equivalents (*What the composition costs*).
The running example is a 4-input classifier on an 8-bit AVR: 44 B and 25 cycles on an ATmega328p.

Parts agree on a small **contract** (a static, pure `proc(in)`, optionally `update(in)`); they do not inherit from a framework, and HAPI's `Chain<>` / `APIOf<>` / `Expand<>` do the
composing. Neural-net cells are the running example, not the point: any static dataflow of pure stages fits (a filter chain, a control loop, a sensor fusion), and other contracts can sit
beside `proc(in)`. Two inference engines are provided as parts, `wave` (no multiplies: shifts, masks and adds) and `lin` (plain integer linear, multiplies by design), and they mix freely in one net.

```cpp
// models/banknote/net.h -- the trained classifier is a TYPE. Its parameters are template arguments: no table, no state, no interpreter.
using BanknoteNet = wave::Cell<WAVE_K,
  wave::Threshold,
  wave::Wave<0,WAVE_F0_N,WAVE_F0_S,WAVE_F0_P,WAVE_F0_M>,  wave::Wave<1,WAVE_F1_N,WAVE_F1_S,WAVE_F1_P,WAVE_F1_M>,
  wave::Wave<2,WAVE_F2_N,WAVE_F2_S,WAVE_F2_P,WAVE_F2_M>,  wave::Wave<3,WAVE_F3_N,WAVE_F3_S,WAVE_F3_P,WAVE_F3_M>>;

bool y = BanknoteNet::proc(features);        // a few shifts, masks and adds; 44 B of flash, 25 cycles on an ATmega328p
```

Not the same result as [`ml_interpreter_cost`](../ml_interpreter_cost): that example is *dispatch removal* (a layered net on ARM without TFLite-Micro's interpreter), which a plain template network
also achieves. This one is *composition*: independent parts, wiring derived from an index, an id or a query, and the realization of a cell (folded or rolled) chosen at compile time. It is measured on an 8-bit chip against a table loop and against emlearn.

## Run it

```sh
pio run -e native && .pio/build/native/program      # static_net banknote: agree 274/274, correct 274/274
pio run -e uno -t upload && pio device monitor       # the same program on an Arduino Uno / Nano (ATmega328p), report over Serial
check/build.sh                                       # the checks: composition, what must not build, sizes, identical disassembly (sugar, by-id), simulated row-by-row
```

`agree` compares the net with the C reference model that trained it, row by row, on the 274 held-out rows of the fold; `correct` compares it with the labels.
Needs a C++17 GNU compiler (g++, clang++, avr-gcc 7.3 or newer) and HAPI 0.7 (`hapi::Expand`); the AVR parts also need avr-gcc, simavr (`measure/`, `check/`) and PlatformIO (`pio`).
The headers use GNU attributes (`[[gnu::always_inline]]`) and the timing harness uses AVR registers: this example is for GCC/Clang, and CI (which compiles `tests/`, MSVC included) does not build it.

## Nets in a few lines

```cpp
// sugar.h: the net as constexpr values whose TYPES are the net (values carry no data). 274 B on an ATmega328p, identical to the same net written by hand.
constexpr auto orr  = cell<-1>(sign, w<2>*a,  w<2>*b);              // a = x<0>, b = x<1>: input slots
constexpr auto nand = cell< 3>(sign, w<-2>*a, w<-2>*b);
constexpr auto xr   = cell<-3>(sign, w<2>*ref(orr), w<2>*ref(nand));   // a cell reads another cell, in place, same pass
using Net = decltype(net(orr, nand, xr));                             // sizeof(Net) == 1

// refid.h: wiring by id or by a query. Cells carry a hapi::Tag<id>; a cell reads another by id, whatever the position and the engine
//   (RW<id,..> is wave::WaveOf<snet::RefId<id>,..>): here a wave cell XORs a wave cell and a lin cell
using ById = snet::Net<
  wave::Cell<0,   hapi::Tag<NAND>, wave::Threshold, wave::Wave<0,0,6,0,0xff>, wave::Wave<1,0,6,0,0xff>>,
  lin::Cell<-1,   hapi::Tag<OR>,   lin::Sign, lin::In<0,2>, lin::In<1,2>>,
  wave::Cell<128, hapi::Tag<XOR>,  wave::Threshold, RW<NAND,0,6,0,0xff>, RW<OR,0,6,0,0xff>>>;
// RefId<id> is RefQ<SameAs<Tag<id>>>: any HAPI predicate over a cell's components works, e.g. RefQ<SameAs<lin::Sign>> is "the cell that has lin::Sign"; the first match wins
```

- **A cycle is a compile error**, not a runtime surprise: `snet::Ref<j>: in-place reference must point to a lower net index; a cycle needs a register (Slot<i> + Store<i>)`.
  The check is a `static_assert` in the net's own library code (`include/staticNet.h`), not something a user of the net writes or can forget: a net with a combinational cycle cannot be evaluated, `proc` does not compile (`check/cycle_reject_*.cpp` name the net and call `proc`).
  A reference to a cell that is not in the net says so in words (`no cell in the net matches Q`, `no cell of that type in the net`) (`check/`: `cycle_reject_*`, `refid_missing`, `sugar_missing`).
- **The realization is a compile-time choice.** `cell<bias>(sign, terms...)` returns the unrolled `lin::Cell` (one inline multiply-add per term) or, from `SUGAR_ROLL_AT` = 6 terms, the same
  terms as a table and a loop (`snet::Roll`); the result is identical, the code is not (see *Realizations* below). `SUGAR_ROLL_AT` is a **size policy**, not a fact: set it huge to never roll.
- Cells of identical type collapse in `ref()` (identical pure cells compute the same value: deduplication, pinned by `check/sugar_twins.cpp`).

## What was measured

Evidence levels, so nothing here claims more than it rests on: **built** (sizes from `avr-size`, instruction counts from `objdump`), **host** (g++/clang++ runs), **simulated** (simavr, a
cycle-accurate simulation of the ATmega328p) and **silicon** (a real chip). All AVR cycle counts below are simavr; `measure/silicon_results.md` ran the same ELFs on a real
Arduino Nano (ATmega328p, 16 MHz) and **all 100 numbers were identical, 0 cycles of difference** (the timed code has no interrupts, caches or wait states, so this is what a cycle-accurate
simulator should give). Sizes are avr-gcc 7.3, `-Os`.

### The classifier, against a table loop and against emlearn (`compare_emlearn/`)

UCI Banknote, 4 features quantized to 8 bits, 5-fold CV; every model is trained on each fold's training rows and scored on that fold's held-out rows; the device programs run fold 1's 274 rows.
One build setup, one harness, flash and RAM net of a no-model program, cycles per call; the on-device correct count equals the host's accuracy for every model.

| model | flash (B) | RAM (B) | cycles | accuracy fold 1 / 5-fold mean (%) |
|---|---|---|---|---|
| **`wave4`** (this example's cell) | **44** | **0** | **25** | 100.00 / 99.27 |
| `lin4` (quantized perceptron, int8 weights) | 112 | 0 | 62 | 99.64 / 99.13 |
| `table4` (same weights, a loop over a PROGMEM table) | 86 | 2 | 127 | 99.64 / 99.13 |
| emlearn tree, depth 2, uint8 features | 232 | 10 | 177 | 92.70 / 90.67 |
| emlearn tree, depth 8, uint8 features | 478 | 10 | 190 | 99.27 / 98.54 |
| emlearn tree, depth 8, float features (its default) | 1374 | 10 | 783 | 98.54 / 98.69 |
| emlearn MLP 4-4-1 (`eml_net`, float) | 4262 | 180 | 12562 | 99.64 / 99.49 |
| emlearn MLP 4-16-1 | 4550 | 564 | 33961 | 99.64 / 99.71 |

At a size near `wave4` there is no emlearn model (the smallest is 5x larger, at 90.7% mean accuracy); trees need depth 8 to reach 98.5%, at about 11x the flash and 7.6x the cycles of `wave4` for 0.7 pp
less accuracy; the MLP is the only model more accurate (+0.15 to +0.44 pp at 4-16 hidden units), at about 97x the flash and 500x the cycles for the 4-hidden-unit net, more for larger ones (soft float on a chip without an FPU).
Cycles of the cells are constant; the trees vary with the path (`compare_emlearn/results.md` has min / mean / max for every model and all depths). **Read with care:** Banknote is nearly separable and says
nothing about a harder task; one dataset; emlearn 0.23.2 as documented (the `dtype='uint8_t'` option for the best-case trees, its float MLP with the 1/255 input scaling folded into layer 0, best of 5 restarts);
emlearn's fixed-point MLP is an unfinished feature upstream and its MLP `inline` method silently emits the loadable code, so no fixed-point MLP was measured (`compare_emlearn/emlearn_repro.py`).

### A wider net: Sonar, 60 inputs (`models/sonar/`, `measure/`)

The `lin` cell of a perceptron on 60 inputs shows why the product width is a parameter (`Prod`): the same net with the multiply at 16 bits (narrow) or 32 bits (wide) is one program on a chip
with a hardware multiplier and another on AVR, which has none for 32 bits.

| cell (fold 0) | flash | AVR code | cycles |
|---|---|---|---|
| narrow, `Prod=int16_t` | 1842 B | 60 inline `mul` | 917 |
| wide, `Prod=int32_t` | 1582 B | 60 calls to `__umulhisi3` / `__muluhisi3` | 2565 |
| table loop, 60 inputs | | | 1545 |

Accuracy (5 folds, 41-42 held-out rows each): the int8 model matches the float perceptron it was quantized from, mean test 74.60% against 74.11%, agreement 99.51% (the difference is one held-out row on fold 2).
The 42 outputs of both cells on fold 0 are **bit-exact with the host row by row**, on a simulated ATmega328p and on the real Nano (`check/sonar_rows_avr.cpp`, `check/bitexact.py`, which also rejects a
single flipped row when the counts agree).

### Realizations: folded or rolled (`measure/roll_sweep.sh`)

The same cell with N terms, unrolled (`lin::Cell`) and rolled (`snet::Roll`, a table and a loop). Flash is a whole tiny program, so only the differences mean anything.

| terms | flash unrolled / rolled (B) | cycles unrolled / rolled |
|---|---|---|
| 1 | 158 / 182 | 14 / 30 |
| 3 | 210 / 234 | 45 / 64 |
| 5 | 238 / 254 | 59 / 117 |
| **6** | **282 / 254** | 67 / 136 |
| 8 | 320 / 262 | 85 / 174 |
| 16 | 440 / 270 | 149 / 326 |
| 32 | 674 / 286 | 284 / 630 |
| 60 | 1042 / 314 | 516 / 1162 |

About 14-16 B per term unrolled against about 3 B rolled, at 2-2.3x the cycles: flash break-even at 5-6 terms, so that is `SUGAR_ROLL_AT`'s default. It is the **size policy**: right on a 328p, where flash runs
out first; a speed-first build never rolls. Only AVR was measured (the break-even will sit elsewhere on a chip with a native multiply-add). Rolled and unrolled agree on every row tried (`check/roll_host.cpp`,
`roll_gaps.cpp`, `sugar_roll.cpp`), and `Roll` refuses at compile time a weight that does not fit int8 or an index that does not fit a byte, instead of truncating.

### What the composition costs

Checked by identical disassembly, not proved. `check/build.sh` builds each pair as a whole program (avr-gcc 7.3, `-Os`, ATmega328p) and compares an md5 of the `avr-objdump -d` output (`same()`); all pass:

| pair | size | identical disassembly |
|---|---|---|
| net written with `sugar.h` / the same net by hand | 274 B | yes |
| net wired by id / the same net wired by index | 304 B | yes |
| `sugar::cell`, rolled / the rolled cell by hand | 322 B | yes |
| `sugar::cell` with `SUGAR_ROLL_AT` huge / the unrolled cell by hand | 1006 B | yes |

Scope: one toolchain, one target, these nets. The `[temp.alias]` argument that `Chain<>::Part` adds nothing does not cover `Drop<>`, `RefId` / `RefQ`, `sugar` or the cycle check; those are what this table measures.
The result also depends on `SNET_INLINE` (`[[gnu::always_inline]]`): avr-gcc 7.3 `-Os` declines to inline the multiply-called `proc` chains without it. Another compiler or target needs the same check, and `build.sh` runs it unchanged.

## Layout

| | |
|---|---|
| `include/` | the headers: `staticNet.h` (the net, `Ctx`, `Ref`, `Slot`; namespace `snet`), `waveCell.h`, `linCell.h` (the engines), `refid.h` (wiring by id / query), `sugar.h`, `roll.h` |
| `models/` | the trained cells: `banknote/` (the running example's cell and its 274 held-out rows), `sonar/` (the 60-input cells of 5 folds), `roll60/` (60-term nets for the size and speed comparisons) |
| `src/main.cpp` | the running example (host and AVR) |
| `check/` | `build.sh` runs everything: host tests (g++, clang++), programs that must not compile, AVR sizes and identical-disassembly checks, the simulated row-by-row check (`bitexact.py`) |
| `measure/` | the timing harness (Timer1, UART) and programs: `run.sh` (simavr), `roll_sweep.sh`, `silicon.py` (flash a real ATmega328p and compare with simavr), `silicon_results.md` |
| `compare_emlearn/` | the Banknote comparison: `run.sh` builds and measures every model in one setup; `results.md`; generated models, the fold data and the emlearn headers it compiles against (MIT, unchanged) |
| `train/` | how the trained parameters came about, on a PC: `bn2x.c` (masked-wave search and float perceptron per fold, UCI Banknote), `sonar_lin_train.c` + `gen_lin_sonar.py` (Sonar, quantized to int8 constants) |

## Scope

This is not an ML framework: there is no training runtime, no operator set, no model loader. Training happens on a PC and produces constants; the net is generated as a type. The contract is what matters, and
other contracts can sit beside `proc(in)` (a gradient or error signal for training, one sample per tick with declared latency for HLS); a net can state which one it requires and HAPI's rules can check that every part
satisfies it. Not covered here: Cortex-M and other targets, any library besides emlearn, and datasets beyond Banknote and Sonar.
