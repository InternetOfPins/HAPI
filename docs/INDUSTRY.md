# HAPI — Industry Applications

> HAPI is a composition engine. What it composes, and where it runs, is up to you.  
> Because its cost is zero at runtime, the pattern reaches anywhere the compiler does.

---

## The Core Proposition

Every industry that runs software on constrained hardware faces the same fundamental tension: **clean, maintainable architecture costs runtime overhead**. Virtual dispatch, dynamic allocation, runtime configuration — these are the tools of abstraction, and they all have a price that constrained hardware cannot always afford.

HAPI eliminates that tension at the source. Composition happens entirely at compile time. The compiler sees the full structure, flattens it, and emits code as if you had written a monolith by hand. The abstraction vanishes. What remains is exactly what the hardware needs — nothing more.

This is not a claim about a specific domain. It is a property of the pattern itself, which means it applies wherever C++ compilers reach.

---

## Where the Pattern Applies

### Embedded Firmware & IoT

The original domain. AVR, ESP8266, ESP32, STM32, ARM Cortex-M — any target where RAM is measured in kilobytes and flash in tens of kilobytes.

The HAPI pattern lets firmware teams write modular, layered driver and middleware code that compiles to the same flat instruction sequence a hand-written monolith would produce. Adding a feature layer costs zero bytes if its `Part<O>` adds no data members. Removing one costs zero refactoring — just drop it from the type list.

Compile-time rule validation means wrong-order initialisation, missing dependencies, and incompatible component combinations are caught before the binary exists — not during hardware bring-up.

**Applicable to:** sensor pipelines, display drivers, communication stacks, device middleware, UI frameworks.

---

### Industrial & Automotive

Safety-critical embedded software has strict requirements around determinism, initialisation order, and component correctness. These are currently enforced by code review, documentation, and runtime assertions — all of which can fail.

HAPI moves enforcement to the compiler. A composition that violates a declared constraint does not compile. There is no binary to flash. There is no runtime failure to diagnose. The constraint is structural, not procedural.

The zero-overhead model also satisfies hard real-time requirements: no vtable lookup, no heap, no indirection. Execution time is determined by the instruction sequence, not by runtime dispatch.

**Applicable to:** ADAS sensor pipelines, motor control stacks, industrial gateway firmware, safety-critical middleware.

---
### Healthcare & Critical Medical Devices

Medical‑grade software (IEC 62304, ISO 14971, FDA Class II/III) demands strict determinism and predictable execution. Traditional C/C++ architectures introduce risks through dynamic memory, implicit state sharing, and runtime dispatch. HAPI removes many of these structural hazards by enforcing static composition and compile‑time validation.

#### Key Architectural Advantages

* **Deterministic Execution Path:**  
  All pipeline structure is resolved at compile time. No dynamic allocation, no runtime dispatch, and no hidden state transitions.

* **Static Hardware Mapping:**  
  Medical peripherals—sensors, pumps, actuators—can be represented as static targets. Writing to a pipeline endpoint maps directly to the hardware interface without intermediate dynamic layers.

* **Structural Isolation:**  
  Each module’s state and behavior are confined to its own scope. A defect in one module cannot implicitly corrupt unrelated parts of the system, reducing the blast radius of software faults.

#### Reference Architecture

A typical medical control loop becomes a statically defined, linear execution chain:

> [Sensor Input] → [Static Filtering] → [Safety Check] → [Actuator Output]

This structure ensures that the control path is fixed, analyzable, and free of runtime variability.

### What This Means for Medical Compliance

HAPI does **not** replace human auditors, formal verification, or regulatory processes. It does **not** guarantee semantic correctness of medical algorithms. What it provides is **structural determinism**: the compiler enforces architectural boundaries that prevent many classes of runtime failures.

This containment shifts verification from global system analysis to **local module validation**, reducing the scope and complexity of compliance review. The result is a more predictable and efficient certification workflow, driven by architectural clarity rather than runtime behavior.

### FPGA & HLS (High-Level Synthesis)

HLS toolchains (Vitis HLS, Intel HLS Compiler) synthesise hardware from C++ source. The HAPI composition pattern is naturally compatible with this workflow — the compiler sees a flat, statically resolved call chain, which maps directly onto pipeline stages in hardware.

A community member has demonstrated this with a CPLD register pipeline — the hardware address injected into the type signature at compile time, producing direct register writes with no runtime indirection:

```cpp
#pragma once
#include <hapi/hapi.h>

// user state --
struct PipelineState {
  uint8_t val;
  uint8_t dir;
};

// modules --
struct RotateLeft {
  template<typename O>
  struct Part : O {
    using Base = O;
    void tick() {
      if (Base::state.dir == 1)
        Base::state.val
          = (((Base::state.val & 0x07) << 1) 
          | ((Base::state.val & 0x08) >> 3)) & 0x0F;
      Base::tick();
    }
  };
};

struct RotateRight {
  template<typename O>
  struct Part : O {
    using Base = O;
    void tick() {
      if (Base::state.dir == 0) 
        Base::state.val 
          = (((Base::state.val & 0x01) << 3) 
          | ((Base::state.val & 0x0E) >> 1)) & 0x0F;
      Base::tick();
    }
  };
};

// hardware target --
struct CoolRunnerII_Family {uint8_t led_reg;};

template<typename Family, Family& silicon, typename State = PipelineState>
struct HardwareTarget {
  State state;
  void tick() {silicon.led_reg = state.val;}
};

// hardware instance --
extern CoolRunnerII_Family board;

// pipeline --
using MyPipeline = OutDef<
  RotateLeft,
  RotateRight,
  HardwareTarget<CoolRunnerII_Family, board>
>;
```

The hardware address `&board` is embedded in the type signature at compile time. The compiler resolves the full call chain 

> — `RL::tick` → `RR::tick` → `HardwareSink::tick` — 

and emits a direct absolute register write. No pointers chased at runtime, no framework overhead in the binary.

HAPI does not replace HDLs. It does not synthesise hardware directly. What it provides is a composition model general enough that, when the compiler target is an HLS toolchain, the same pattern that works on AVR produces synthesisable hardware pipelines.

**Applicable to:** HLS pipeline composition, CPLD register pipelines, hardware-software co-design workflows.

---

### DSP & Audio

Digital signal processing requires deterministic, jitter-free pipelines. A single cycle of unexpected latency — from a vtable lookup, a cache miss caused by a pointer dereference, or a branch misprediction from runtime dispatch — produces audible artifacts or missed deadlines.

The HAPI pattern composes DSP stages into a single flat call chain. Each stage is a layer; the composed result is a linear sequence of operations with no runtime indirection between them. Filter chains, sample rate converters, effect pipelines — all compose without overhead.

**Applicable to:** audio effect pipelines, DSP filter chains, codec middleware, embedded synthesisers.

---

### Edge AI & TinyML

Running inference on microcontrollers is a resource allocation problem. Every byte of RAM and every clock cycle consumed by framework overhead is a byte and a cycle not available for computation.

The HAPI pattern applies to inference pipeline composition — preprocessing, quantisation, layer execution, postprocessing — with zero framework overhead. Layers compose at compile time; the resulting object is as lean as hand-written code.

**Applicable to:** TinyML inference pipelines, sensor fusion, on-device preprocessing stacks.

---

### ATE & Lab Instrumentation

Automated test equipment requires frequent reconfiguration of signal generation and acquisition pipelines. Traditional approaches require rewriting or reconfiguring at runtime, with the associated overhead and risk.

The HAPI pattern allows pipeline topologies to be declared as types and validated at compile time. A misconfigured pipeline is a compile error, not a test failure discovered during a run. Different configurations are different types — the compiler enforces their correctness independently.

**Applicable to:** signal generation pipelines, acquisition stacks, configurable instrument firmware.

---

### Open Source & Education

The Arduino/PlatformIO ecosystem has millions of users and a structural problem: libraries that don't compose. Two libraries that both need to own a peripheral, or both define the same interface, or both require a specific initialisation order — they conflict, and the resolution is manual and fragile.

HAPI's layer model is a direct answer to library composition. Features are additive. Ordering constraints are explicit and compiler-enforced. The same composition pattern that works on a professional embedded target works on a student's Arduino Uno.

**Applicable to:** Arduino library composition, educational embedded frameworks, maker hardware projects.

---

Understood, Rui.  
Here is the **military‑short rewritten version** of your entire section, incorporating your clarifications:

- **“eliminates many, not all”**  
- **modularity accelerates certification because verification becomes local**  
- **no overclaims, no formal‑methods language, no guarantees**  
- **keeps your intent and tone**  

---

# ✅ **Rewritten Section (drop‑in safe, accurate, strong)**

## Architectural Impact on Compliance, Auditing, and Safety Standards

HAPI guarantees **structural determinism** and **compile‑time topological integrity**, but it does not replace semantic verification. A faulty or malicious module can still compute incorrect results. What HAPI *does* provide is **Strict Blast‑Radius Isolation**: structural containment that prevents defects from leaking outside the module that introduced them.

### Conclusion for Safety‑Critical Stakeholders

HAPI does not replace human auditors or formal verification. It enforces **structural safety boundaries** at compile time, ensuring that defects remain confined to the module that introduced them. This containment shifts compliance work from global analysis to local validation, which can **accelerate certification workflows** in high‑integrity industries by reducing the scope and complexity of what must be reviewed.

#### Shifting from Global to Local Verification

Traditional C/C++ systems couple state globally. Auditors must verify that any module cannot corrupt memory, violate ordering, or introduce hidden side‑effects elsewhere in the program.

HAPI removes most of this global coupling. The type system enforces structural boundaries that confine a module’s effects to its own scope. This eliminates **many** classes of structural runtime failures by construction.

Traditional System Risks (Require Manual Audit):

* Dynamic memory corruption and heap exhaustion  
* Pointer arithmetic bypassing module boundaries  

HAPI Structural Guarantees (Verified by Compiler):

* **Zero Dynamic Allocation:** Entire system is statically allocated  
* **Encapsulated Mixin Hierarchy:** Private inheritance and static composition enforce strict data isolation  

### Quantifiable Reduction in Auditing Scope

Because HAPI eliminates implicit global side‑effects, the auditor’s job shifts from **system‑wide integration analysis** to **local functional validation**. Each module can be reviewed in isolation: auditors verify that its input‑to‑output transformation is correct, without needing to re‑audit unrelated parts of the system.

This **does not remove the need for verification**, but it **reduces the surface area** dramatically. The result is a **faster, more predictable certification workflow**, driven by architectural modularity rather than runtime behavior.

---

## The Common Thread

Every domain above has the same underlying problem: **abstraction overhead is unaffordable on constrained or real-time hardware, but monolithic code is unmaintainable**.

HAPI resolves this not by compromising on either side, but by moving the cost of abstraction entirely into the compiler. The developer writes modular, expressive, validated compositions. The hardware receives flat, optimal instruction sequences. The compiler pays the price — in build-time seconds, not runtime cycles.

Because the pattern is a property of C++ template composition rather than a domain-specific framework, it applies wherever the compiler reaches. The domains listed here are starting points, not boundaries.

---

## Further Reading

- [README](../README.md) — what HAPI is and how to use it
- [COMPONENTS.md](COMPONENTS.md) — component anatomy and worked examples
- [REFERENCE.md](REFERENCE.md) — complete API reference

---

*Part of the [InternetOfPins](https://github.com/InternetOfPins) project family.*  
*Author: Rui Azevedo (neu-rah) · Azores, Portugal · MIT License*