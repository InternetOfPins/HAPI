# HAPI — Industry Applications

> HAPI is a composition engine. What it composes, and where it runs, is up to you.
>
> Because its cost is paid at compile time rather than runtime, the pattern applies anywhere a C++ compiler reaches.

---

# The Core Proposition

Every industry that runs software on constrained, real-time, or high-integrity systems faces the same tension:

> Clean, maintainable architecture usually comes with runtime cost.

Virtual dispatch, runtime registration, dynamic allocation, dependency injection, and runtime configuration are valuable tools, but they consume resources and introduce additional complexity that must be understood, tested, and maintained.

HAPI eliminates that tradeoff by moving composition into the type system.

The compiler sees the complete structure of the system, validates declared constraints, and resolves the composition into a flat implementation. The abstraction disappears from the generated binary.

The result is software that can remain modular, expressive, and reusable while introducing no mandatory runtime composition overhead.

This is not a property of any particular industry. It is a property of the composition model itself.

---

# Where the Pattern Applies

## Embedded Firmware & IoT

The original domain.

AVR, ESP8266, ESP32, STM32, RP2040, ARM Cortex-M, and similar platforms operate under strict memory and performance constraints. Every byte of RAM and every cycle of execution matters.

The HAPI pattern allows firmware teams to build layered drivers, middleware, communication stacks, and user interfaces without introducing runtime composition overhead.

Features are added by composition rather than modification. Layers can be inserted, removed, or replaced without restructuring the rest of the system.

Compile-time validation ensures that declared dependencies and ordering constraints are checked before a binary is produced.

### Applicable To

* Device drivers
* Hardware abstraction layers
* Sensor pipelines
* Communication stacks
* Display systems
* Embedded user interfaces
* Middleware frameworks

---

## Industrial & Automotive

Industrial control systems and automotive software place a premium on determinism, predictability, and traceability.

Many architectural constraints are traditionally enforced through documentation, code review, testing, and engineering discipline. HAPI allows these constraints to be expressed directly in code and verified during compilation.

A composition that violates a declared rule simply fails to compile.

The architecture introduces no mandatory runtime dispatch, heap allocation, or framework-level indirection, making execution behavior easier to reason about in time-sensitive environments.

### Applicable To

* Motor-control systems
* Industrial gateways
* Factory automation
* ADAS preprocessing pipelines
* Vehicle communication stacks
* Embedded control middleware

---

## Telecommunications & Protocol Stacks

Communication systems are naturally layered.

```text
Application
→ Serialization
→ Framing
→ Encryption
→ Error Detection
→ Transport
→ Physical Interface
```

Traditional implementations often distribute responsibilities across large classes, runtime registration systems, or tightly coupled middleware.

HAPI models protocol layers as compile-time components. Each layer performs a specific transformation and forwards the result to the next stage. The compiler sees the complete communication path and resolves it into a flat call chain.

### Architectural Advantages

#### Explicit Protocol Topology

The complete protocol stack is visible in a single type declaration.

#### Compile-Time Layer Validation

Dependencies, ordering constraints, and incompatibilities can be expressed as composition rules and enforced by the compiler.

#### Encourages Component Isolation

Protocol layers can maintain private internal state and expose only explicit interfaces. By leveraging standard C++ access control, implementation details remain localized to the component that defines them, helping reduce unintended coupling between layers.

#### Zero Runtime Composition Cost

Serialization, framing, checksums, routing, and transport layers compose without virtual dispatch, runtime registration, or dynamic allocation.

### Applicable To

* CAN
* Modbus
* NMEA
* MQTT
* Telemetry systems
* Industrial field buses
* Custom serial protocols
* Embedded networking stacks
* SDR control paths

---

## Robotics & Autonomous Systems

Robotic systems are typically built from deterministic processing pipelines.

```text
Sensor
→ Filter
→ Localization
→ Planning
→ Control
→ Actuator
```

As systems grow, maintaining clear boundaries between stages becomes increasingly important.

Hidden coupling, shared mutable state, and implicit dependencies can make behavior difficult to understand and changes difficult to assess.

HAPI models each stage as an explicit compile-time component. Processing order, dependencies, and constraints become part of the system structure rather than runtime convention.

### Architectural Advantages

#### Explicit Control Topology

The complete processing pipeline is visible in a single type declaration.

#### Compile-Time Validation

Dependencies and ordering constraints can be expressed as composition rules and enforced by the compiler.

#### Encourages Component Isolation

Components can maintain private internal state and expose only explicit interfaces. By leveraging standard C++ access control, implementation details remain localized to the component that defines them, helping reduce unintended coupling between stages.

#### Deterministic Execution

No runtime composition overhead, dynamic allocation requirements, or framework-level dispatch mechanisms are introduced by the architecture.

### Applicable To

* Industrial robotics
* CNC controllers
* Motion-control systems
* Autonomous vehicles
* Sensor-fusion pipelines
* Embedded robotics middleware

---

## Healthcare & Medical Devices

Medical software often prioritizes predictability, traceability, and architectural clarity.

Dynamic allocation, runtime configuration, and implicit coupling can complicate analysis, testing, and maintenance.

HAPI composes systems statically. Component relationships, ordering constraints, and dependencies are visible to the compiler and validated before a binary is produced.

The resulting execution path is fixed and deterministic, with no framework-level runtime dispatch or allocation.

### Architectural Advantages

#### Explicit System Structure

The complete architecture is visible through composition rather than runtime configuration.

#### Compile-Time Validation

Declared structural constraints are verified before deployment.

#### Encourages Component Isolation

HAPI leverages standard C++ access control and compile-time composition to encourage component-local state ownership. Components can maintain private internal state and expose only explicit interfaces, reducing hidden coupling and making the impact of changes easier to analyze.

#### Predictable Execution

The execution path is fixed by the compiled composition rather than runtime discovery or registration.

### What This Means for Compliance

HAPI does not replace testing, risk analysis, formal verification, certification processes, or regulatory review.

Its contribution is architectural.

By making component boundaries explicit and reducing hidden coupling, HAPI can reduce architectural uncertainty and narrow the scope of impact analysis when systems evolve over time.

### Applicable To

* Patient monitoring systems
* Diagnostic instruments
* Laboratory equipment
* Portable medical devices
* Embedded control systems

---

## FPGA & High-Level Synthesis (HLS)

Modern HLS toolchains synthesize hardware from C++ source.

Because HAPI compositions are statically resolved, they naturally align with the pipeline-oriented structure common in hardware design.

The compiler sees the complete call graph and can flatten the resulting implementation into a deterministic processing chain.

HAPI does not generate HDL and does not replace traditional hardware-description languages.

Its contribution is providing a composition model that maps naturally onto synthesis-oriented workflows.

### Applicable To

* HLS pipeline composition
* CPLD register pipelines
* FPGA host-side APIs
* Hardware-software co-design
* Register-map generation frameworks

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

## DSP & Audio

Digital signal-processing systems often operate under tight latency and jitter constraints.

```text
Input
→ Filter
→ Transform
→ Analyze
→ Output
```

Because HAPI contributes no runtime composition overhead, available processor time can be dedicated entirely to signal-processing work rather than framework infrastructure.

In resource-constrained designs, eliminating framework overhead can enable lower clock rates, smaller devices, reduced power consumption, or more complex processing pipelines within the same real-time budget.

HAPI does not make DSP algorithms faster.

It removes itself from the cycle budget.

### Applicable To

* Audio effect chains
* DSP filter pipelines
* Embedded synthesizers
* SDR preprocessing
* Codec middleware
* Sensor-processing systems

---

## Edge AI & TinyML

Running machine-learning workloads on microcontrollers is fundamentally a resource-allocation problem.

Every byte of RAM and every clock cycle consumed by framework infrastructure is unavailable for application logic.

The HAPI pattern applies naturally to the stages surrounding inference:

```text
Sensor
→ Calibration
→ Filtering
→ Feature Extraction
→ Quantization
→ Inference
→ Postprocessing
→ Decision
```

These stages can be composed and validated at compile time while introducing no framework-level runtime overhead.

HAPI does not accelerate neural-network kernels themselves.

Its contribution is architectural: organizing the stages surrounding inference into explicit, compile-time validated processing pipelines.

### Applicable To

* TinyML systems
* Sensor-fusion pipelines
* Edge inference devices
* Smart sensors
* On-device preprocessing systems

---

## ATE & Laboratory Instrumentation

Automated test equipment and scientific instrumentation often require configurable acquisition and processing pipelines.

A misconfigured pipeline may not fail until a test run is already underway.

With HAPI, pipeline topology becomes part of the type system.

Different configurations become different types, allowing the compiler to validate each independently.

### Applicable To

* Signal generation systems
* Acquisition pipelines
* Oscilloscopes
* Logic analyzers
* Spectrum analyzers
* Scientific instrumentation firmware

---

## Open Source & Education

The Arduino and maker ecosystems frequently encounter library-composition problems.

Libraries compete for peripherals, require specific initialization orders, or expose incompatible interfaces.

HAPI's layer model provides a framework for additive composition.

Dependencies become explicit, ordering constraints become compiler-enforced, and integration issues can be detected before deployment.

The same pattern that scales to industrial firmware remains accessible to educational and hobbyist projects.

### Applicable To

* Arduino ecosystems
* Educational frameworks
* Maker projects
* Teaching modern C++ design
* Embedded-software experimentation

---

# Additional High-Integrity Domains

The industries described above are not exhaustive.

HAPI's architectural properties become increasingly valuable as systems become more deterministic, resource-constrained, safety-critical, or operationally expensive to validate.

As the cost of failure increases, the value of explicit composition, compile-time validation, deterministic execution, and localized component boundaries tends to increase as well.

Potential application areas include:

* Power generation and distribution systems
* Electrical grid monitoring and control
* Railway signaling and control systems
* Mass-transit infrastructure
* Aerospace and avionics
* Defense systems
* Maritime navigation and control
* Energy management systems
* Building automation and industrial SCADA
* Scientific instrumentation
* Environmental monitoring networks
* Satellite and ground-station infrastructure

HAPI does not guarantee correctness, safety, security, or regulatory compliance.

Those properties depend on system design, implementation, verification, and operational procedures.

What HAPI contributes is architectural determinism: explicit component boundaries, compile-time composition, and the elimination of framework-level runtime composition overhead.

In domains where failures can be expensive, disruptive, or dangerous, these properties can simplify reasoning about system structure and reduce architectural complexity.

---

# The Common Thread

Every domain above faces the same fundamental challenge:

> Abstraction overhead is undesirable, but monolithic software is difficult to maintain.

HAPI resolves this by moving composition into the compiler.

Developers work with modular, expressive, reusable components.

The compiler validates structure, resolves composition, and emits a flat implementation.

The hardware receives only the behavior that remains after optimization.

The domains described here are examples, not boundaries.

Wherever software can be expressed as deterministic processing stages, layered transformations, or validated component compositions, the HAPI pattern can apply.

---

# Further Reading

* README.md — What HAPI is and how to use it
* COMPONENTS.md — Component anatomy and implementation patterns
* REFERENCE.md — Complete API reference

---

Part of the InternetOfPins project family.

Author: Rui Azevedo (neu-rah)
Azores, Portugal
MIT License
