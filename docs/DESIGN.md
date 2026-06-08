# DESIGN.md

## HAPI & OneData Architecture Blueprint

### Zero-Overhead, Static Meta-Programming for High-Reliability Embedded Systems

## High-Reliability Design Guidelines

To ensure that custom modules and ecosystem extensions do not compromise system stability, their development is governed by two strict engineering principles:

Elimination of Runtime Allocation: No dynamic memory allocation within the core module architecture. Everything must live on the stack or in static storage.

Controlled Control Flow: Recursive paths are banned across all modules. Loops and complex branching must be isolated to high-level orchestration layers (e.g., menus or communication drivers) to protect fast-path execution units from stack bloat.

---

## 2. Architectural Topology

OneData achieves extreme composability without dynamic polymorphism by using a static linear inheritance chain (CRTP/Variadic Mixins via `hapi::APIOf`).

```
+-------------------------------------------------------+
|                 Component API Front                   | (e.g., DataDef<Watch, NumRange<int>, Data<int>>)
+-------------------------------------------------------+
                           |
                           v  [Linear Compilation Cascade]
+-------------------------------------------------------+
|             Layer 1: Modifier (Watch)                 |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|             Layer 2: Validator (NumRange)             |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|             Layer 3: Storage (Data/DataRef)           |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|             Base Layer: DataAPI / hapi::Nil           |
+-------------------------------------------------------+

```

Unlike traditional Object-Oriented layouts that utilize runtime Object Lists (`OneList` models) causing virtual table (`vtable`) pointer bloat and cache-miss overhead, OneData expands structures directly at the definition point.

---

## 3. Structural Mechanics & Stack Safety

Operating within ultra-minimalist environments (such as 8-bit AVR architecture with under 2KB of SRAM) requires absolute control over the Stack. When the Stack collides with static memory, the system fails silently.

### High-Risk Boundary: The Inline Barrier

The compiler's `inline` and `constexpr` mechanisms are heuristic recommendations. When a code path encounters **loops (`for`, `while`)** or heavy branching, the compiler will refuse to inline, generating a real hardware subroutine call (`CALL`/`RCALL`).

To enforce safety, OneData modules adhere to strict design rules:

#### Rule I: The Loop-Isolation Principle

Intermediate nodes within a static mixin chain must never contain execution loops or dense conditional matrixes.

* **Bad:** A data modifier that loops internally to filter data during transmission.
* **Good:** Pushing all iterative logic to the terminal leaf of execution, or isolating it inside an explicit, non-inline boundary away from the fast-path configuration.

#### Rule II: Pass-by-Value for Register Optimization

Mutator interfaces (such as `set(Type v)`) handling scalar or hardware register-mapped primitives must pass data **by value** rather than by reference-to-const (`const Type&`).

* **Reasoning:** Passing `volatile` references to literals forces the compiler to construct temporary shadow variables on the Stack. Pass-by-value allows the compiler to bind parameters directly into CPU working registers (e.g., `R24/R25` on AVR), bypassing the stack frame completely.

---

## 4. Qualification & Binary Inspection Protocol

To qualify a HAPI/OneData module as "Production-Ready" and robust, developers must perform binary-level validation. Do not trust high-level code layout; audit the machine output.

### Step 1: Static Memory Mapping

Verify that compile-time components (`StaticData`, `StaticNumRange`, `DataRef`) contribute exactly `0` bytes to the final `.data` and `.bss` RAM allocations.

```bash
# Verify static allocation footprint using size tools
avr-size --format=avr --mcu=atmega328p firmware.elf

```

### Step 2: Assembly Tree Auditing

Dump the object disassembly to verify that cascading hardware mutations (like writing to a pin or setting a reference register) compiled down into atomic machine instructions without runtime overhead.

```bash
# Audit the generated code for un-inlined subroutines (CALL/RCALL)
avr-objdump -S firmware.elf | grep -A 10 "led_pin"

```

* **Success Criteria:** The call site must collapse into direct `STS` (Store to Space) or `OUT` commands. There must be no tracking frame pointers, pushing, or popping registers for basic state mutation.

### Step 3: Worst-Case Stack Analysis (WCSA)

Compile the test and operational suite using GCC's stack analysis flags to map the exact execution depth of cascading calls (especially deep serialization trees like `print(Out& out)`).

```bash
# Enforce stack generation metrics
PROJECT_CXXFLAGS = -fstack-usage -flto

```

Analyze the generated `.su` files to guarantee that the deepest calling sequence fits safely within the target hardware's hardware stack boundary under worst-case system interrupts.