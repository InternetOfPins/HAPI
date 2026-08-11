# HAPI — Industry Applications

> HAPI is a composition engine. What it composes, and where it runs, is up to you.  
> Because its cost is paid at compile time rather than runtime, the pattern applies anywhere a C++ compiler reaches.

---

## The Core Proposition

Every industry that runs software on constrained, real-time, or high-integrity systems faces the same tension:

> Clean, maintainable architecture usually comes with runtime cost.

Virtual dispatch, runtime registration, dynamic allocation, and runtime configuration are valuable tools — but they consume resources and introduce complexity that must be understood, tested, and maintained.

HAPI eliminates that tradeoff by moving composition into the type system. The compiler sees the complete structure, validates declared constraints, and resolves the composition into a flat implementation. The abstraction disappears from the generated binary.

The result is software that remains modular, expressive, and reusable while introducing no mandatory runtime composition overhead.

This is not a property of any particular industry. It is a property of the composition model itself.

---

## Architectural Properties

Before examining specific domains, these are the properties HAPI contributes — and what it does not claim.

| Property | HAPI Guarantee |
|---|---|
| Runtime composition overhead | Zero — all folding happens at compile time |
| Dynamic allocation | None introduced by the framework |
| Virtual dispatch | None introduced by the framework |
| Composition validity | Declared constraints verified at compile time |
| Component data isolation | Guaranteed by standard C++ access control, scoped to the specific `Chain<>` instantiation — `private` members are reachable only within their own layer; `protected`/`public` members are reachable by layers above, by declared choice. |
| Algorithm correctness | Not guaranteed — depends on implementation |
| Regulatory compliance | Not guaranteed — depends on system design and verification |

HAPI contributes **architectural determinism**: explicit component boundaries, compile-time composition, and the elimination of framework-level runtime overhead. Correctness, safety, and compliance remain the responsibility of the system designer.

---

## Structural Verification & Safety-Critical Systems

This section is placed first because it is the most differentiated property of the HAPI pattern for high-integrity domains.

In traditional C++ architectures, auditing a system for safety-critical compliance is expensive because state can be globally coupled. Auditors must verify that any module cannot produce unintended side effects elsewhere — heap exhaustion, pointer corruption, unexpected control flow.

HAPI's composition model constrains structural scope at the type level:

- **No dynamic allocation** introduced by the framework — eliminates heap exhaustion as a framework-level risk
- **No runtime registration** — component relationships are fixed at compile time and visible in the source
- **Compile-time constraint validation** — declared ordering rules, dependencies, and incompatibilities are checked before a binary exists
- **Explicit composition topology** — the complete system structure is readable from a single type declaration

Layers are encouraged to keep internal state `private`; standard C++ access control then confines it absolutely to that layer, for the lifetime of the specific `Chain<>` instantiation — no other layer, above or below, can reach it. `protected` and `public` members remain reachable by layers above, by declared choice — that choice, not its enforcement, is the design discipline. For any given layer, the reachable set is exactly the layers above it in that one composition, enumerable by reading the `Chain<>` type declaration.

This narrows the auditor's scope. Compile-time validation closes off structural integration risk — allocation, dispatch, undeclared coupling, ordering — on top of local functional correctness: does this component transform its input correctly within its execution sequence? Composed, emergent behaviour across the full chain still requires verification, same as any system; HAPI narrows what must be re-checked each time, it does not remove the need to check the whole sequence.

HAPI does not replace human verification. It narrows the structural surface that requires it.

**Relevant standards:** ISO 26262 (automotive), IEC 62304 (medical), DO-178C (avionics), MISRA C++.  
HAPI introduces no dynamic allocation, virtual dispatch, or runtime framework overhead — properties that align with MISRA C++ objectives but do not constitute compliance. Compliance depends on the complete system design, implementation, and verification process.

---

## Where the Pattern Applies

### Embedded Firmware & IoT

The original domain. AVR, ESP8266, ESP32, STM32, RP2040, ARM Cortex-M — platforms where every byte and every cycle matters.

HAPI allows firmware teams to build layered drivers, middleware, communication stacks, and interfaces without runtime composition overhead. Features are added by composition rather than modification. Compile-time validation ensures declared dependencies are checked before a binary is produced.

**Applicable to:** device drivers, hardware abstraction layers, sensor pipelines, communication stacks, display systems, embedded UI, middleware frameworks.

---

### Industrial & Automotive

Determinism, predictability, and traceability are priorities. Architectural constraints traditionally enforced through documentation and code review can instead be expressed in code and verified at compilation. A composition that violates a declared rule fails to compile.

No dynamic allocation or runtime dispatch is introduced by the framework, making execution behavior easier to reason about in time-sensitive environments.

**Applicable to:** motor control, industrial gateways, factory automation, ADAS preprocessing, vehicle communication stacks.

---

### Telecommunications & Protocol Stacks

Communication systems are naturally layered:

```
Application → Serialisation → Framing → Encryption → Error Detection → Transport → Physical
```

HAPI models protocol layers as compile-time components. The complete protocol stack is visible in a single type declaration. Dependencies, ordering constraints, and incompatibilities can be expressed as composition rules and enforced by the compiler. Serialisation, framing, checksums, and transport layers compose without virtual dispatch or dynamic allocation.

**Applicable to:** CAN, Modbus, NMEA, MQTT, telemetry systems, industrial field buses, custom serial protocols, embedded networking stacks.

---

### Robotics & Autonomous Systems

Robotic systems are built from deterministic processing pipelines:

```
Sensor → Filter → Localisation → Planning → Control → Actuator
```

HAPI models each stage as an explicit compile-time component. Processing order, dependencies, and constraints become part of the system structure rather than runtime convention. No runtime composition overhead, dynamic allocation, or framework-level dispatch is introduced.

**Applicable to:** industrial robotics, CNC controllers, motion control, autonomous vehicles, sensor fusion pipelines.

---

### Healthcare & Medical Devices

Medical software prioritises predictability, traceability, and architectural clarity. Dynamic allocation and implicit coupling complicate analysis and maintenance.

HAPI composes systems statically. Component relationships, ordering constraints, and dependencies are visible to the compiler and validated before a binary is produced. The execution path is fixed by the compiled composition rather than runtime discovery.

**Applicable to:** patient monitoring, diagnostic instruments, laboratory equipment, portable medical devices, embedded control systems.

---

### Hardware Pipeline Synthesis

Software pipelines built with HAPI collapse at compile time into instruction sequences that are architecturally equivalent to hardware pipelines: sequential, deterministic, no runtime dispatch, no indirection.

The ATmega328P evaluation (see the published paper) demonstrates this directly. The synthesised pipeline consists exclusively of immediate-load and displacement-store instructions. Hardware peripheral control values are embedded as compile-time constants and written to I/O registers without any intermediate pointer dereference, stack frame, or virtual table lookup. The compiler produces what a hardware designer would produce by hand.

This means HAPI pipelines are not *abstractions over* hardware — they *are* the hardware sequence, expressed in modular C++. The abstraction cost is paid once at compile time; the hardware receives the result.

A minimal, live example: [a CRC-6 chain on Compiler Explorer](https://godbolt.org/z/d5Y5Gc44M) — a stack of stateless HAPI components collapses into a straight-line, branchless instruction sequence, with no call, jump, or virtual dispatch anywhere in the disassembly.

```
HAPI composition → compiler synthesis → flat register sequence
```

The same model scales from 8-bit AVR registers to multi-core ARM peripherals. The pipeline topology is fixed in the type; the register addresses are compile-time template parameters; the instruction stream is the compiler's output.

**Applicable to:** hardware abstraction layers, peripheral drivers, register-mapped control pipelines, device initialisation sequences, hardware co-design firmware.

---

### FPGA & CPLD Register Interfacing

Industrial systems often use CPLDs or FPGAs as register-mapped bus bridges. The pattern maps naturally to hardware register pipelines — hardware addresses as compile-time template parameters, embedded directly in the type signature.

HAPI's compile-time composition layer (`Chain`, `APIOf`) is synthesizable by a high-level-synthesis (HLS) toolchain, verified against [PandA-Bambu](https://github.com/ferrandi/PandA-bambu) 2024.10 (clang16 frontend). [`examples/hls_smoke`](../examples/hls_smoke) is a runnable, verified proof: a 5-layer `Chain<>`/`APIOf<>` composition synthesized end-to-end into real Verilog RTL. The recursive inheritance collapse produces hardware proportional to the underlying computation — the example's 5 composed layers collapse to a single adder in the generated RTL, with no structural bloat from the composition depth. As with any HLS input, code reachable from the synthesis entry point has to stay within what the backend can map to hardware — I/O and OS calls have no synthesis target, in HAPI or in any other HLS flow.

HAPI does not generate HDL directly and is not itself a synthesis engine — it produces plain, flattened C++ that an HLS toolchain can consume like any other input. Only Bambu has been verified so far; other HLS toolchains and frontends are untested, not confirmed working. Hardware validation on physical fabric also remains ongoing; `examples/hls_smoke` validates the synthesis path, not deployment on a specific device.

<details>
<summary>Zero-Overhead Compile-Time Composition Example — 4-Tap FIR Filter</summary>

This is the real synthesis target from [`examples/hls_fir`](../examples/hls_fir), not a hypothetical. A 4-tap FIR filter is built as `Chain<Tap<1>, Tap<3>, Tap<3>, Tap<1>>`: each `Tap<Coeff>` layer owns its own delay register (`z`) and folds into a single multiply-accumulate call chain, verified end-to-end through Bambu HLS. With coefficients as compile-time NTTPs (as below), Bambu's scheduler strength-reduces every tap's multiply into shift+add — `Estimated number of DSPs: 0`. The identical composition reading coefficients from a runtime table instead synthesizes a real `mult_expr_FU`-backed multiplier (`Estimated number of DSPs: 2`, time-shared across all four taps). Full verified numbers in [`examples/hls_fir/README.md`](../examples/hls_fir/README.md#results-verified-not-estimated).

```cpp
#include <hapi/hapi.h>
#include <cstdint>
using namespace hapi;

struct Item {
  static int32_t mac(int16_t /*delayed*/, int32_t acc) { return acc; }
};

template<int16_t Coeff>
struct Tap {
  template<typename I>
  struct Part : I {
    using Base = I;
    using Base::Base;
    int16_t z{0};

    int32_t mac(int16_t x, int32_t acc) {
      int32_t sum  = acc + static_cast<int32_t>(Coeff) * static_cast<int32_t>(z);
      int16_t prev = z;
      z = x;
      return I::mac(prev, sum);
    }
  };
};

using Fir4 = Chain<Tap<1>, Tap<3>, Tap<3>, Tap<1>>;
using Top  = APIOf<Item, Fir4>;

Top fir4;

int32_t firTop(int16_t x) {
  return fir4.mac(x, 0);
  // The compiler flattens Tap<1>::mac → Tap<3>::mac → Tap<3>::mac → Tap<1>::mac
  // into a straight-line multiply-accumulate chain. No virtual dispatch, no
  // heap, each tap's delay register folded into its own flip-flops.
}
```
</details>

**Applicable to:** CPLD register pipelines, FPGA host-side APIs, hardware-software co-design, register-map abstraction frameworks, HLS-targeted compile-time pipelines.

---

### DSP & Audio

DSP systems operate under tight latency and jitter constraints. HAPI does not make DSP algorithms faster. It removes itself from the cycle budget.

Available processor time is dedicated entirely to signal-processing work. In resource-constrained designs, eliminating framework overhead can enable lower clock rates, smaller devices, reduced power consumption, or more complex pipelines within the same real-time budget.

[`examples/hls_fir`](../examples/hls_fir) extends the HLS verification above from stateless composition to a stateful one: an N-tap FIR filter where each tap is a `Chain<>` layer owning its own delay register, synthesized end-to-end through the same Bambu flow. It also verifies both sides of a real DSP-inference question: with compile-time coefficients, Bambu's own scheduler strength-reduces every tap's multiply into shift+add (zero DSPs); with the identical filter reading coefficients from a runtime table instead, a genuine DSP-mappable multiplier is synthesized (`mult_expr_FU`/`Estimated number of DSPs: 2`), time-shared across all four taps. Which one a real design gets depends entirely on whether its coefficients are fixed at compile time or configurable at runtime — both are verified, not assumed.

**Applicable to:** audio effect chains, DSP filter pipelines, embedded synthesisers, SDR preprocessing, codec middleware.

---

### Edge AI & TinyML

Running inference on microcontrollers is a resource-allocation problem. HAPI applies to the deterministic management pipelines surrounding inference:

```
Sensor → Calibration → Filtering → Feature Extraction → Quantisation → Inference Input
```

These stages compose and validate at compile time with no framework-level runtime overhead. HAPI does not accelerate neural-network kernels or execution engines. Its contribution is structural — organising the pipelines feeding into and processing data from inference models.

**Applicable to:** sensor fusion for edge devices, smart sensor data chains, on-device preprocessing.

---

### ATE & Laboratory Instrumentation

A misconfigured test pipeline may not fail until a test run is already underway. With HAPI, pipeline topology becomes part of the type system. Different configurations become different types, and the compiler validates each independently.

**Applicable to:** signal generation, acquisition pipelines, oscilloscopes, logic analysers, spectrum analysers, scientific instrumentation firmware.

---

### Open Source & Education

The Arduino and maker ecosystems frequently encounter library-composition problems — libraries competing for peripherals, requiring specific initialisation orders, or exposing incompatible interfaces.

HAPI's layer model makes dependencies explicit and ordering constraints compiler-enforced. The same pattern that scales to industrial firmware remains accessible to educational and hobbyist projects.

**Applicable to:** Arduino ecosystems, educational frameworks, maker projects, teaching modern C++ design.

---

## High-Integrity Domain Summary

| Domain | Key Properties Used |
|---|---|
| Embedded / IoT | Zero overhead, composability, portability |
| Industrial / Automotive | Determinism, compile-time validation, no dynamic allocation |
| Telecommunications | Layered composition, ordering constraints, zero dispatch overhead |
| Robotics | Deterministic pipelines, explicit topology, stage isolation |
| Medical Devices | Static composition, predictable execution, structural traceability |
| Hardware Pipeline Synthesis | Compile-time collapse to hardware-equivalent instruction sequences |
| FPGA / CPLD | Compile-time address embedding, zero-overhead register abstraction, verified Bambu HLS synthesis |
| DSP / Audio | No framework overhead in cycle budget |
| Edge AI / TinyML | Zero-overhead preprocessing pipelines |
| ATE / Instrumentation | Type-safe pipeline configuration, compile-time topology validation |
| Education / OSS | Accessible composability, explicit dependencies |

---

## Additional Domains

The list above is not exhaustive. HAPI's properties become increasingly valuable as systems become more deterministic, resource-constrained, safety-critical, or operationally expensive to validate.

Potential application areas include power generation and distribution, railway signalling, mass-transit infrastructure, aerospace and avionics (subject to toolchain qualification), maritime navigation, building automation, environmental monitoring, and satellite infrastructure.

---

## The Common Thread

> Abstraction overhead is undesirable, but monolithic software is difficult to maintain.

HAPI resolves this by moving composition into the compiler. Developers work with modular, expressive, reusable components. The compiler validates structure, resolves composition, and emits a flat implementation. The hardware receives only the behaviour that remains after optimisation.

The domains described here are examples, not boundaries. Wherever software can be expressed as deterministic processing stages, layered transformations, or validated component compositions, the HAPI pattern can apply.

---

## Further Reading

- [README](../README.md) — What HAPI is and how to use it
- [COMPONENTS.md](COMPONENTS.md) — Component anatomy and implementation patterns
- [REFERENCE.md](REFERENCE.md) — Complete API reference
- [Live Godbolt example](https://godbolt.org/z/d5Y5Gc44M) — CRC-6 HAPI chain collapsing to branchless assembly

---

*Part of the [InternetOfPins](https://github.com/InternetOfPins) project family.*  
*Author: Rui Azevedo (neu-rah) · Azores, Portugal · MIT License*
