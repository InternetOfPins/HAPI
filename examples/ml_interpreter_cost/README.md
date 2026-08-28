# ml_interpreter_cost

What HAPI removes from an on-device inference stack is **the interpreter**.

This example runs one small neural network — TFLite-Micro's shipped
`hello_world` model, a three-layer fully-connected sine approximator
(`FC 1→16 +ReLU`, `FC 16→16 +ReLU`, `FC 16→1`) — two ways on the same MCU:

- **`net_a.h`** — the network as a HAPI `Chain<>` of layer components. The
  chain collapses to one flat function. No graph, no arena, no dispatch.
- **`tflm.h`** — the *identical* network (same trained weights) through
  TFLite-Micro's `MicroInterpreter`: a flatbuffer model blob, a fixed
  tensor arena, an op resolver, and a per-operator dispatch loop.

They compute the same function to float rounding (`check/build.sh`). What
differs is everything around the arithmetic.

HAPI does not make the multiply-accumulates faster — the reference kernels
do the same FLOPs. It removes the machinery that reads a serialized graph
at runtime and walks it.

## Results — verified, not estimated

Built with PlatformIO. STM32 is `bluepill_f103c8` / `framework=cmsis` /
arm-none-eabi-gcc 12.3 (the default gcc 7.2 predates the C++17 TFLM needs).
ESP32 is `esp32dev` / `framework=arduino`.

### STM32F103 — 64 KB flash, 20 KB RAM

| build | flash | RAM |
|---|---:|---:|
| `netA_bluepill` (HAPI `Chain<>`) | **2 944 B** (4.5 %) | **40 B** (0.2 %) |
| `tflm_float_bluepill` | **38 768 B** (59.2 %) | **5 264 B** (25.7 %) |
| `tflm_int8_bluepill` | 38 364 B (58.5 %) | 5 264 B (25.7 %) |

Same network: TFLite-Micro is **13× the flash and 130× the RAM**, and takes
**most of the chip** (59 % / 26 %) to run a net HAPI runs in 4.5 % / 0.2 %.
int8 barely helps — the interpreter, not the arithmetic, is the cost.

TFLM's RAM is a **4 096 B tensor arena** (the model needs ~2.6 KB;
`kArenaSize` in `tflm.h`) plus interpreter/allocator state. TFLM's flash:
`micro_interpreter` 6.7 KB, `micro_interpreter_graph` 6.2 KB,
`micro_allocator` 4.9 KB, `micro_op_resolver` 3.8 KB, `micro_allocation_info`
3.5 KB, `kernel_util` 2.6 KB, ~10 smaller ≈ 5 KB, the FullyConnected kernel,
flatbuffer parsing, and a 3 164 B model blob in `.rodata`. HAPI's: ~1.6 KB
of net code, no arena, weights inline in flash.

### ESP32 — 4 MB flash, 320 KB RAM (framework floor dominates the totals)

| build | flash | RAM | Δ flash | Δ RAM |
|---|---:|---:|---:|---:|
| `netA_esp32` | 234 677 B | 21 040 B | — | — |
| `tflm_float_esp32` | 266 265 B | 25 436 B | **+31 588 B** | **+4 396 B** |
| `tflm_int8_esp32` | 265 853 B | 25 436 B | +31 176 B | +4 396 B |

### The dispatch

`MicroInterpreterGraph::InvokeSubgraph`, once per operator (ARM disassembly,
resolved against relocations):

```
ldr.w  fp, [r5, #28]     ; fp = &node.registration        (TFLMRegistration*)
ldr.w  r3, [fp, #12]     ; r3 = registration->invoke      -- a function
cbnz   r3, ...            ;      pointer loaded from data; NO relocation
blx    r3                 ; <-- the operator dispatch. genuine indirect.
```

`hello_world` has three `FullyConnected` ops → three such indirect
dispatches per inference (plus two profiler virtual calls each), on top of
~50 more indirect calls in the one-time `AllocateTensors` flatbuffer walk.
The whole TFLM firmware carries 86 `blx <reg>`.

The HAPI build's object has **zero** indirect calls — every call is a
soft-float ABI helper (`__aeabi_fmul`/`fadd`) or, on Xtensa, a
`-mlongcalls`-encoded *static* target. The five-layer `Chain<>` inlines to
one leaf function.

### Correctness and hardware

`check/build.sh` — native, `net_a::infer` vs `tflm::infer` across a sweep of
`x ∈ [0, 2π]`, agree to < 1e-4 (max Δ ≈ 3e-7). Flashed to a real STM32 Blue
Pill (OpenOCD RAM read) and a real ESP32 (`*_esp32_hw`, serial): both nets
return the same value — `y = 0.98165` for `x = 1.5` on the ESP32;
`0.9816483` (HAPI) vs `0.9816480` (TFLM, 4 ULP from FC-kernel op order) on
the STM32.

AVR is not a target: TFLite-Micro needs a C++ standard library the AVR
toolchain doesn't ship, plus ~35 KB of interpreter and a multi-KB arena —
several times the whole atmega328.

## Building

`lib/tflm/` (the TFLite-Micro source tree) is **not** in the repo — it is
~5 MB of third-party code. Generate it once:

```
tools/get_tflm.sh          # needs git + make + network. NOT TensorFlow.
```

This clones `tensorflow/tflite-micro` at a pinned revision and runs its
`create_tflm_tree.py` generator. The model files (`models/*.tflite`,
`models/*_model.h`) and the extracted HAPI weights (`hello_world_weights.h`)
are checked in and already match that revision — see `models/PROVENANCE.md`.

```
export PATH="$HOME/.platformio/penv/bin:$PATH"
pio run -e netA_bluepill
pio run -e tflm_float_bluepill        # 59 % of flash -- fits, but only just
pio run -e {netA,tflm_float,tflm_int8}_esp32
pio run -e {netA,tflm_float}_esp32_hw # + serial readback
bash check/build.sh                   # native equivalence check
```

## Files

| file | |
|---|---|
| `net_a.h` | the network as a HAPI `Chain<Fc, Relu, Fc, Relu, Fc>` |
| `tflm.h` | the same network via `tflite::MicroInterpreter` |
| `src/{netA,tflm}.cpp` | per-target entry points (`-DHW_READBACK` adds serial) |
| `hello_world_weights.h` | trained weights as `constexpr float` arrays (generated) |
| `models/` | the two `.tflite` files + `xxd -i` C arrays + `PROVENANCE.md` |
| `check/` | native equivalence check + minimal TFLM source list |
| `tools/get_tflm.sh` | regenerate `lib/tflm/` |
| `tools/extract_weights.py` | regenerate `hello_world_weights.h` from the `.tflite` |
