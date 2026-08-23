# rust_stm32_bridge

Real embedded Rust firmware calling into a real, unmodified HAPI-composed
C++ stack — on real STM32F103C8 (Blue Pill) hardware, the same board and
chip `focCompose`'s C++ work already hardware-verified. Unlike `config_loader`/
`cutlass_layout`/`cuda_device_chain` (all "does the composition mechanism
generalize past embedded" proofs), this is the reverse move: a real
embedded consumer, the audience a go-to-market plan for this technology
would actually target.

**Built in three phases, each verified on real hardware before moving on**
(not "it compiled" — real OpenOCD register readback at every step, the
same discipline every hardware round in this project uses):

## Phase A — Rust owns the chip

`src/main.rs`, pure Rust (`cortex-m-rt` + `stm32f1xx-hal`), zero HAPI/C++.
Drives PC13 (onboard LED, active-low) low. Real OpenOCD readback:
`GPIOC.CRH` bits for PC13 = `0011` (push-pull output, matching
`into_push_pull_output` exactly), `GPIOC.ODR` bit 13 = `0`. Purpose:
isolate "does Rust genuinely control this exact chip" from "does the
bridge work," before any FFI complexity enters.

## Phase B — the C++ shim, compiled and linked

**Why not `cxx` or `bindgen` directly** (real research, not assumed):
- `cxx` (the standard safe Rust↔C++ bridge) added `no_std` support in
  1.0.58, but it **requires a global allocator** (`features=["alloc"]`)
  — true heap-free `core`-only support was explicitly deprioritized by
  its maintainer and never shipped (confirmed via
  [dtolnay/cxx#933](https://github.com/dtolnay/cxx/issues/933)). In
  tension with this ecosystem's own no-dynamic-allocation stance.
- `bindgen` cannot bind C++ templates beyond field access on
  non-specialized types — no template methods, no monomorphization, no
  concept of `hapi::Chain<>`'s recursive inheritance fold (confirmed via
  its own docs, rust-bindgen/cpp.html). Anything it can't translate
  becomes an opaque byte blob.
- The Rust embedded book's own guidance: *"As C++ does not have a stable
  ABI for the Rust compiler to target, it is recommended to use the `C`
  ABI when combining Rust with C or C++."*

So `cpp/shim.cpp` does exactly that: composes a trivial `Counter`+
`DoubleStep` component via real, unmodified `hapi::APIOf<>` (the SAME
`Chain<>` inheritance-fold + template-template-parameter pattern used in
`HAPI/examples/cuda_device_chain`, cross-compiled here instead with the
real `arm-none-eabi-g++` toolchain `focCompose` already uses — not a new
toolchain), storing state in a `static` (no heap), and exposes it through
two flat `extern "C"` functions. `build.rs` invokes that same real
compiler directly (not the `cc` crate — its target-detection heuristics
don't know `thumbv7m-none-eabi`) and links the result into the Rust
binary.

**Real result: links completely clean.** No symbol
conflicts, no vector-table clash (Rust's `cortex-m-rt` owns the reset
handler/vector table; the C++ shim contributes no startup code of its
own, since `Ticker`'s default constructor is trivial — zero-init only,
confirmed via the final `.bss` placement, no runtime constructor call
needed). `arm-none-eabi-nm` on the final binary confirms both
`hapi_ticker_inc`/`hapi_ticker_get` at real flash addresses and
`g_ticker` in `.bss` RAM.

**Zero-overhead, confirmed at `-Os`, same as every other target in this
project (AVR/ESP32/ARM/PTX):** the compiled shim's `hapi_ticker_inc()` is
a single `ldr`/`adds #2`/`str` — `DoubleStep`→`Counter`'s two-layer fold
collapses to one add, zero internal calls. **But the FFI seam itself is
honestly NOT zero-overhead** — confirmed both by research (real,
ABI-mandated, non-inlined calls; cross-language LTO exists but the
`rustc` book's own description of practical use is manual bitcode
extraction/relinking, no bare-metal precedent found) and empirically
here: `arm-none-eabi-objdump` on the final binary shows three real `bl`
instructions from Rust's `main` into `hapi_ticker_inc` and one into
`hapi_ticker_get`. HAPI's zero-overhead property holds *inside* the C++
side; it stops at the language boundary. Reported plainly, not glossed
over — the same discipline as `cutlass_layout`'s `reference::host::Gemm`
finding.

## Phase C — flashed and verified on real hardware

Real flash+verify via the same ST-Link/OpenOCD flow as Phase A/
`focCompose` (device id `0x20036410` — the same physical board).
`main.rs` calls `hapi_ticker_inc()` three times (`DoubleStep`→2×
`Counter::inc()` per call, `3×2=6` — matching `cuda_device_chain`'s
numbers exactly, for a direct side-by-side comparison across toolchains)
then drives PC13 low only if the C++ side's answer is exactly `6`.

**Real OpenOCD readback, post-flash:**
```
0x20000004 (g_ticker.value, RAM):  0x00000006   <- the C++-composed value, live
0x40011004 (GPIOC.CRH):            0x44344444   <- PC13 = push-pull output
0x4001100c (GPIOC.ODR):            0x00000000   <- PC13 driven low (LED on)
```
The value genuinely crossed the FFI boundary correctly and drove real
hardware state — not "the firmware didn't crash," a direct register-level
confirmation of the actual computed result.

## Build & flash

```sh
cargo build --release
~/.platformio/packages/tool-openocd/bin/openocd \
  -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program target/thumbv7m-none-eabi/release/rust_stm32_bridge verify reset exit"
```

`build.rs` defaults to PlatformIO's bundled `arm-none-eabi-g++`/`-ar`
(`~/.platformio/packages/toolchain-gccarmnoneeabi/bin/`) and this
repo's own `../../include` for HAPI's headers — override with the
`ARM_GXX`/`ARM_AR`/`HAPI_INCLUDE` env vars if those paths don't apply
elsewhere.

## Requirements

- `rustup` + `rustup target add thumbv7m-none-eabi` (official installer,
  no root needed).
- A real `arm-none-eabi-g++`/`-ar` (PlatformIO's bundled one works, or
  any other — see `ARM_GXX`/`ARM_AR` above).
- Real ST-Link + OpenOCD to flash (same setup `focCompose`'s hardware
  rounds already use).

## What this doesn't prove, and why that's honest

This tests the mechanism on a trivial component, not `focCompose`'s real
SPI sensor/PWM driver composition — swapping in a real slice of that
work is a natural next step, but deliberately sequenced after the
mechanism itself was proven on something small (the same "trivial but
real" discipline `cuda_device_chain` used before this). And per the FFI
finding above: don't expect this pattern to preserve zero-overhead across
a *hot, high-frequency* Rust↔C++ boundary (e.g. calling into a composed
FOC control loop every PWM cycle) without measuring the real call
overhead first — it's a real cost here, just a very small one for a
handful of calls at startup.
