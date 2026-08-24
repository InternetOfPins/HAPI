# HAPI — Industry Applications

> HAPI is a composition engine. What it composes, and where it runs, is up to you.  
> Because its cost is paid at compile time rather than runtime, the pattern applies anywhere a C++ compiler reaches.

---

## How to Read This Document

Every domain section below is tagged with one of two statuses, stated
plainly rather than left to the reader to infer from tone:

- **Demonstrated** — a real example exists in this repository or a
  sibling `InternetOfPins` repo, with a working build and, where
  synthesis is involved, verified output. The section names the example.
- **Potential** — the architectural argument (compile-time composition,
  no dynamic allocation, no runtime dispatch) plausibly extends to this
  domain, but nothing has been built or tested here. Worth exploring,
  not a claim of readiness.

A third distinction applies specifically to regulated, safety-critical
domains (medical devices, automotive, aerospace, rail): even where the
architecture is sound potential, adoption there faces a **certification
barrier** that compile-time composition doesn't touch — documented
lifecycle process, risk management, and toolchain qualification, per
IEC 62304 / ISO 26262 / DO-178C / EN 50128. See
[Structural Verification & Safety-Critical Systems](#structural-verification--safety-critical-systems)
below. A property that *aligns with* those standards' objectives is not
compliance with them, and no amount of architectural elegance substitutes
for the qualification work itself.

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

**Relevant standards:** ISO 26262 (automotive), IEC 62304 (medical), DO-178C (avionics), EN 50128 (rail), MISRA C++.
HAPI introduces no dynamic allocation, virtual dispatch, or runtime framework overhead — properties that align with MISRA C++ objectives but do not constitute compliance. None of these standards has been pursued for this project: no tool qualification, no documented safety lifecycle, no certification audit. Every domain section below that touches a regulated industry links back to this paragraph rather than repeating it — that link is doing real work, not a formality.

---

## Where the Pattern Applies

### Embedded Firmware & IoT

**Status: Demonstrated.** HAPI's original domain and its most exercised one — OneMenu, OneChip, OneIO, and `examples/iotComposition` are real, shipped, cross-toolchain-verified components, not architectural argument alone.

AVR, ESP8266, ESP32, STM32, RP2040, ARM Cortex-M — platforms where every byte and every cycle matters.

HAPI allows firmware teams to build layered drivers, middleware, communication stacks, and interfaces without runtime composition overhead. Features are added by composition rather than modification. Compile-time validation ensures declared dependencies are checked before a binary is produced.

**Applicable to:** device drivers, hardware abstraction layers, sensor pipelines, communication stacks, display systems, embedded UI, middleware frameworks.

---

### Industrial & Automotive

**Status: Potential.** Nothing in this section has been built in an automotive or industrial-control harness. A closely related technique — compile-time-composed motor control — has been verified on real hardware, but in a robotics context (see Robotics & Autonomous Systems below), not an automotive ECU one; treat that as an architectural precedent, not evidence for this section specifically.

Determinism, predictability, and traceability are priorities. Architectural constraints traditionally enforced through documentation and code review can instead be expressed in code and verified at compilation. A composition that violates a declared rule fails to compile.

No dynamic allocation or runtime dispatch is introduced by the framework, making execution behavior easier to reason about in time-sensitive environments.

**Applicable to:** motor control, industrial gateways, factory automation, vehicle communication stacks. **ADAS preprocessing** additionally carries the certification barrier described above (ISO 26262) — architectural fit here is a smaller claim than deployment readiness.

---

### Telecommunications & Protocol Stacks

**Status: Potential.** Unbuilt in this codebase — no protocol stack has been composed with HAPI end to end. Included because the layering maps cleanly onto `Chain`'s composition model, not because it's been tried.

Communication systems are naturally layered:

```
Application → Serialisation → Framing → Encryption → Error Detection → Transport → Physical
```

HAPI models protocol layers as compile-time components. The complete protocol stack is visible in a single type declaration. Dependencies, ordering constraints, and incompatibilities can be expressed as composition rules and enforced by the compiler. Serialisation, framing, checksums, and transport layers compose without virtual dispatch or dynamic allocation.

**Applicable to:** CAN, Modbus, NMEA, MQTT, telemetry systems, industrial field buses, custom serial protocols, embedded networking stacks.

---

### Robotics & Autonomous Systems

**Status: Partially demonstrated.** Motion control and sensor fusion are real: a compile-time-composed motor-control integration has been verified on real hardware, and a sensor-fusion pipeline has been verified on AVR (`examples/iotComposition`). Industrial robotics and CNC controllers as complete systems are unbuilt extrapolations from that work, not separately verified. **Autonomous vehicles is a materially larger claim than either** — full perception/planning/control stacks, not a single composed control loop — and additionally carries the certification barrier described above; listing it alongside motion control understates the gap between them.

Robotic systems are built from deterministic processing pipelines:

```
Sensor → Filter → Localisation → Planning → Control → Actuator
```

HAPI models each stage as an explicit compile-time component. Processing order, dependencies, and constraints become part of the system structure rather than runtime convention. No runtime composition overhead, dynamic allocation, or framework-level dispatch is introduced.

**Applicable to:** industrial robotics, CNC controllers, motion control, autonomous vehicles, sensor fusion pipelines.

---

### Healthcare & Medical Devices

**Status: Potential.** Nothing built or tested in a medical-device context. The certification barrier described above applies in full here: IEC 62304 requires a documented software lifecycle, risk management (ISO 14971), traceability from requirement to code to test, and qualification of the toolchain itself. None of that exists for this project today, and no architectural property below shortens that path.

Medical software prioritises predictability, traceability, and architectural clarity. Dynamic allocation and implicit coupling complicate analysis and maintenance.

HAPI composes systems statically. Component relationships, ordering constraints, and dependencies are visible to the compiler and validated before a binary is produced. The execution path is fixed by the compiled composition rather than runtime discovery.

**Applicable to:** patient monitoring, diagnostic instruments, laboratory equipment, portable medical devices, embedded control systems.

---

### Hardware Pipeline Synthesis

**Status: Demonstrated.** The ATmega328P evaluation (published paper) and the live Godbolt CRC-6 example below are real, reproducible builds, not description.

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

**Status: Demonstrated.** OneHLS's Fir/Biquad/Pid/ComplexMac components are synthesized and verified under both Vitis HLS and Bambu HLS on real Artix-7 post-route results — see the OneHLS repository.

<details>
<summary>Example: compile-time FIR tap collapsing to register-mapped pipeline</summary>

```cpp
template<int32_t Coeff>
struct Tap {
  template<typename I>
  struct Part : I {
    using Base = I;
    using Base::Base;
    int32_t mac(int16_t x, int32_t acc) {
      return I::mac(x, acc + Coeff * x);
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

**Status: Partially demonstrated.** `Fir<>` and `Biquad<>` are real, synthesized, and verified in OneHLS (see `examples/hls_fir`). Audio effect chains, embedded synthesisers, SDR preprocessing, and codec middleware are unbuilt extrapolations from those two primitives, not separately verified.

DSP systems operate under tight latency and jitter constraints. HAPI does not make DSP algorithms faster. It removes itself from the cycle budget.

Available processor time is dedicated entirely to signal-processing work. In resource-constrained designs, eliminating framework overhead can enable lower clock rates, smaller devices, reduced power consumption, or more complex pipelines within the same real-time budget.

[`examples/hls_fir`](../examples/hls_fir) extends the HLS verification above from stateless composition to a stateful one: an N-tap FIR filter where each tap is a `Chain<>` layer owning its own delay register, synthesized end-to-end through the same Bambu flow. It also verifies both sides of a real DSP-inference question: with compile-time coefficients, Bambu's own scheduler strength-reduces every tap's multiply into shift+add (zero DSPs); with the identical filter reading coefficients from a runtime table instead, a genuine DSP-mappable multiplier is synthesized (`mult_expr_FU`/`Estimated number of DSPs: 2`), time-shared across all four taps. Which one a real design gets depends entirely on whether its coefficients are fixed at compile time or configurable at runtime — both are verified, not assumed.

**Applicable to:** audio effect chains, DSP filter pipelines, embedded synthesisers, SDR preprocessing, codec middleware.

---

### Edge AI & TinyML

**Status: Demonstrated.** A Dense→ReLU→Dense inference pipeline and its surrounding sensor/calibration/feature-extraction stages are verified on real AVR hardware.

Running inference on microcontrollers is a resource-allocation problem. HAPI applies to the deterministic management pipelines surrounding inference:

```
Sensor → Calibration → Filtering → Feature Extraction → Quantisation → Inference Input
```

These stages compose and validate at compile time with no framework-level runtime overhead. HAPI does not accelerate neural-network kernels or execution engines. Its contribution is structural — organising the pipelines feeding into and processing data from inference models.

**Applicable to:** sensor fusion for edge devices, smart sensor data chains, on-device preprocessing.

---

### ATE & Laboratory Instrumentation

**Status: Potential.** Unbuilt — no test-and-measurement pipeline has been composed with HAPI. Included on architectural analogy to the verified pipeline-composition work elsewhere in this document, not on its own evidence.

A misconfigured test pipeline may not fail until a test run is already underway. With HAPI, pipeline topology becomes part of the type system. Different configurations become different types, and the compiler validates each independently.

**Applicable to:** signal generation, acquisition pipelines, oscilloscopes, logic analysers, spectrum analysers, scientific instrumentation firmware.

---

### Open Source & Education

**Status: Demonstrated.** This is the project's own lineage — ArduinoMenu's history and OneMenu's real-world use are exactly this domain, not an analogy to it.

The Arduino and maker ecosystems frequently encounter library-composition problems — libraries competing for peripherals, requiring specific initialisation orders, or exposing incompatible interfaces.

HAPI's layer model makes dependencies explicit and ordering constraints compiler-enforced. The same pattern that scales to industrial firmware remains accessible to educational and hobbyist projects.

**Applicable to:** Arduino ecosystems, educational frameworks, maker projects, teaching modern C++ design.

---

### General-Purpose Application Software

**Status: Demonstrated.** OneParse benchmarks its runtime parsing throughput against real desktop libraries, and `examples/config_loader` is a working, non-embedded CLI tool built on HAPI + OneData + OneParse — this section describes something running, not a hypothesis.

The pattern was designed for embedded and industrial firmware, but nothing
in `Chain`, `Part`, or `APIOf` is hardware-specific — the identical
composition model applies to any C++17 program. HAPI's own compile-time
benchmark against Boost.Hana measures a concern (template-instantiation
cost) that matters equally outside embedded code. OneParse, a library built
on HAPI, already benchmarks its runtime parsing throughput against desktop
parsing libraries (lexy, PEGTL, simdjson, rapidjson, Boost.Spirit.X3) —
independent of any embedded target. `examples/config_loader` (in the HAPI
repository) is the first example combining HAPI, OneData, and OneParse
together with no embedded target at all: a CLI config loader/validator.

**Applicable to:** CLI tools, configuration/schema validation, build
tooling, desktop application composition, server-side configuration
loaders.

---

## High-Integrity Domain Summary

| Domain | Key Properties Used | Status |
|---|---|---|
| Embedded / IoT | Zero overhead, composability, portability | Demonstrated |
| Industrial / Automotive | Determinism, compile-time validation, no dynamic allocation | Potential (ADAS: certification barrier) |
| Telecommunications | Layered composition, ordering constraints, zero dispatch overhead | Potential |
| Robotics | Deterministic pipelines, explicit topology, stage isolation | Partial (motion control/sensor fusion demonstrated; autonomous vehicles: certification barrier) |
| Medical Devices | Static composition, predictable execution, structural traceability | Potential — certification barrier |
| Hardware Pipeline Synthesis | Compile-time collapse to hardware-equivalent instruction sequences | Demonstrated |
| FPGA / CPLD | Compile-time address embedding, zero-overhead register abstraction, verified Bambu HLS synthesis | Demonstrated |
| DSP / Audio | No framework overhead in cycle budget | Partial (Fir/Biquad demonstrated) |
| Edge AI / TinyML | Zero-overhead preprocessing pipelines | Demonstrated |
| ATE / Instrumentation | Type-safe pipeline configuration, compile-time topology validation | Potential |
| Education / OSS | Accessible composability, explicit dependencies | Demonstrated |

---

## Additional Domains

The list above is not exhaustive. HAPI's properties become increasingly valuable as systems become more deterministic, resource-constrained, safety-critical, or operationally expensive to validate. All of the following are potential, unbuilt fit — none has a HAPI example today.

Power generation and distribution, maritime navigation, building automation, environmental monitoring, and satellite infrastructure fall in the same category as Telecommunications and ATE above: plausible, unverified, no regulatory barrier beyond ordinary engineering practice.

Railway signalling, mass-transit infrastructure, and aerospace/avionics belong instead with Healthcare and Automotive/ADAS above — architecturally plausible, but subject to the same certification barrier (EN 50128 for rail, DO-178C for avionics) described in Structural Verification & Safety-Critical Systems. Toolchain qualification, not composition elegance, is the gating step for any of these three.

---

## The Common Thread

> Abstraction overhead is undesirable, but monolithic software is difficult to maintain.

HAPI resolves this by moving composition into the compiler. Developers work with modular, expressive, reusable components. The compiler validates structure, resolves composition, and emits a flat implementation. The hardware receives only the behaviour that remains after optimisation.

The domains described here are examples, not boundaries. Wherever software can be expressed as deterministic processing stages, layered transformations, or validated component compositions, the HAPI pattern can apply — at the status stated for each, above.

---

## Further Reading

- [README](../README.md) — What HAPI is and how to use it
- [COMPONENTS.md](COMPONENTS.md) — Component anatomy and implementation patterns
- [REFERENCE.md](REFERENCE.md) — Complete API reference
- [Live Godbolt example](https://godbolt.org/z/d5Y5Gc44M) — CRC-6 HAPI chain collapsing to branchless assembly

---

*Part of the [InternetOfPins](https://github.com/InternetOfPins) project family.*  
*Author: Rui Azevedo (neu-rah) · Azores, Portugal · MIT License*