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
| Component data isolation | Design discipline — layers can keep data `private`, but `protected` and `public` members remain accessible to layers above via inheritance. Full isolation is not guaranteed. |
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

Layers are encouraged to keep internal state `private`, limiting direct access to the layer that owns it. Because layers compose through inheritance however, `protected` and `public` members remain accessible to layers above. Full blast-radius isolation is a design discipline, not a structural guarantee provided by HAPI.

This shifts the auditor's scope. Rather than verifying global integration properties, they can focus on local functional correctness — does this component transform its input correctly within its execution sequence?

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

```
HAPI composition → compiler synthesis → flat register sequence
```

The same model scales from 8-bit AVR registers to multi-core ARM peripherals. The pipeline topology is fixed in the type; the register addresses are compile-time template parameters; the instruction stream is the compiler's output.

**Applicable to:** hardware abstraction layers, peripheral drivers, register-mapped control pipelines, device initialisation sequences, hardware co-design firmware.

---

### FPGA & CPLD Register Interfacing

Industrial systems often use CPLDs or FPGAs as register-mapped bus bridges. HAPI maps naturally to hardware register pipelines — the hardware address is a compile-time template parameter, embedded directly in the type signature.

HAPI does not generate HDL and does not replace hardware-description languages or fabric synthesis engines. Its contribution is an elegant, zero-overhead abstraction layer for software components interfacing with register-mapped logic.

<details>
<summary>Zero-Overhead Register-Mapped Interface Example</summary>

```cpp
#include <hapi/hapi.h>
using namespace hapi;

struct CoolRunnerII_Family { uint8_t led_reg; };

struct RotateLeft {
  template<typename O>
  struct Part : O {
    using Base = O;
    void tick(uint8_t val, uint8_t dir) {
      if (dir == 1)
        val = (((val & 0x07) << 1) | ((val & 0x08) >> 3)) & 0x0F;
      Base::tick(val, dir);
    }
  };
};

struct RotateRight {
  template<typename O>
  struct Part : O {
    using Base = O;
    void tick(uint8_t val, uint8_t dir) {
      if (dir == 0)
        val = (((val & 0x01) << 3) | ((val & 0x0E) >> 1)) & 0x0F;
      Base::tick(val, dir);
    }
  };
};

template<typename Family, Family& silicon>
struct HardwareSink {
  template<typename O>
  struct Part : O {
    void tick(uint8_t val, uint8_t dir) {
      silicon.led_reg = val;  // direct register write — address resolved at compile time
    }
  };
};

extern CoolRunnerII_Family board;

using Pipeline = APIOf<SinkAPI, RotateLeft, RotateRight, HardwareSink<CoolRunnerII_Family, board>>;
Pipeline pipeline;

void on_clock_tick(uint8_t direction) {
  pipeline.tick(board.led_reg, direction);
  // The compiler flattens RotateLeft::tick → RotateRight::tick → HardwareSink::tick
  // into a direct absolute register write. No pointers chased at runtime.
}
```
</details>

**Applicable to:** CPLD register pipelines, FPGA host-side APIs, hardware-software co-design, register-map abstraction frameworks.

---

### DSP & Audio

DSP systems operate under tight latency and jitter constraints. HAPI does not make DSP algorithms faster. It removes itself from the cycle budget.

Available processor time is dedicated entirely to signal-processing work. In resource-constrained designs, eliminating framework overhead can enable lower clock rates, smaller devices, reduced power consumption, or more complex pipelines within the same real-time budget.

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
| FPGA / CPLD | Compile-time address embedding, zero-overhead register abstraction |
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

---

*Part of the [InternetOfPins](https://github.com/InternetOfPins) project family.*  
*Author: Rui Azevedo (neu-rah) · Azores, Portugal · MIT License*
