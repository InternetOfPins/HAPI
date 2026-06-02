# HAPI

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**A zero-overhead static composition engine for C++.**

Enables modular, type-safe, and high-performance system design through advanced compile-time composition.

---

```cpp
#include <hapi/hapi.h>

// Building a structural compile-time Abstract Syntax Tree (AST)
using MyMenu = MenuDef<
  Title<"Main">,
  StaticBody<
    ItemDef<Id<1>,Action<op1>>,
    ItemDef<Id<2>,Action<op2>>,
    MenuDef<...>>
  >
>;

int main() {
    MyMenu::printMenu(SerialOut);
}
```

---

## Why HAPI?

Modern embedded and systems programming often faces a difficult choice:

- Low-level C offers performance and predictability, but poor structure and maintainability.
- Traditional C++ provides abstraction, but often at the cost of runtime overhead and complexity.

HAPI provides a third path: **zero-overhead static composition**. It lets developers build clean, composable, high-level APIs that the compiler transforms into highly efficient, deterministic code.

---

### Zero-Overhead Pipeline Synthesis (C++17)

HAPI uses C++17 type composition to build hardware pipelines that compile directly into static MMIO instructions. Passing hardware addresses as compile-time template arguments eliminates all runtime objects, pointers, and vtable overhead.

```cpp
// 100% Static Pipeline Architecture
using MyCoolRunnerPipeline = Chain<
  RotateLeft, 
  Chain<RotateRight, 
  CoolRunner2_Target<&board>>>; // Direct C++17 auto template injection

MyCoolRunnerPipeline pipeline;

void on_clock_tick(uint8_t direction) {
  pipeline.state.val = board.led_reg; // Sample state
  pipeline.state.dir = direction;
  pipeline.tick();                   // Fuses logic into MMIO write
}

```

#### Why HAPI Scales:

* **Zero RAM Footprint**: `sizeof(pipeline) == 2` (stores only volatile `val` and `dir` bus states).
* **Direct Register Mutation**: Resolves to an absolute assembly write (`mov` to `&board`), bypassing runtime indirection.
* **Compiler-Driven Netlist**: The static type tree provides an open, deterministic AST structure that external tools can parse directly for logic synthesis.

---

## Core Pillars

- **Static Composition** — Components are assembled at compile time into flat, cache-friendly structures with full inlining.
- **Type-Level Validation** — Structural and semantic rules are verified during compilation using powerful predicates.
- **Zero Runtime Cost** — All composition and validation happens statically. No vtables, minimal indirection, predictable memory layout.
- **Functional Influence** — Strong emphasis on composition, immutability of structure, and making invalid states unpresentable.

---

## Documentation

* **[Component Architecture](docs/COMPONENTS.md)** — How components, structural blocks, and hardware abstraction layers work.
* **[API Reference](docs/REFERENCE.md)** — Core types, patterns, and advanced usage.
* **[HAPI for engineers and industry](docs/INDUSTRY.md)** Why does it matter.
---

**Made with obsession in the Azores** 🇵🇹  
By [Rui Azevedo](https://github.com/neu-rah) • [@ruihfazevedo](https://x.com/ruihfazevedo)

