# HAPI

**A powerful, zero-overhead, static composition engine for modern C++ and embedded systems.**

---

```cpp
#include <hapi/hapi.h>

using namespace hapi;

// Building a structural compile-time Abstract Syntax Tree (AST)
using MyMenu = Menu<Title, StaticBody<Action1, Action2, SubMenu>>;

// The compiler validates and structures the entire layout in contiguous memory
using System = system_bus<MyMenu>;

int main() {
    System::init();
    // Zero pointer-chasing: compiled down to inline, contiguous execution
    MyMenu::printMenu(SerialOut); 
}

```

## Why HAPI?

### The High-Performance Infrastructure Crisis

Embedded software is moving deeper into mission-critical environments—Automotive, Medical Devices, Edge AI, and Industrial IoT Automation. In these domains, sloppy abstractions and runtime bloat are a direct economic hazard.

Companies building next-generation software-defined platforms or safety-certified hardware stacks are hitting a wall. They are caught between two bad options:

* **Legacy C Macros:** Highly efficient, but fragile, unscalable, type-unsafe, and notoriously difficult to maintain.
* **Standard Object-Oriented C++:** Clean and modular, but bogged down by dynamic overhead, virtual function tables (v-tables), and unpredictable runtime memory fragmentation.

`HAPI` eliminates this compromise by serving as a **Zero-Overhead Static Composition Engine**. It allows architects to structure clean, high-level API contracts that the compiler collapses down into deterministic, lightning-fast machine instructions.

---

## Core Pillars & Architectural Blueprint

`HAPI` treats software architecture through a functional lens: **Programs are types that run when instantiated.** By combining modern C++ template metaprogramming with strict functional paradigms, it introduces features previously absent in the embedded space:

### 1. Heterogeneous Compile-Time Trees (`HLists`)

Instead of nesting classes or chasing runtime pointers via object graphs, components (like UI Menus, DSP Pipelines, or Drivers) are composed as static, heterogeneous inductive chains (`StaticBody<O, OO...>`).

* **Perfect Cache Locality:** The compiler flattens your nested layout into a single, contiguous struct in memory. No heap, no fragmentation, and zero pointer overhead.
* **Full Loop Unrolling:** Recursive operations are completely resolved and flattened by the compiler, emitting linear, branchless machine code.

### 2. Type-Level Pattern Matching

Using highly optimized fold-expression engines, `HAPI` allows your software to execute structural queries directly on your type system at compile time:

```cpp
constexpr const bool has_parser = query<IsDataParser, MyComposition>;

```

The architecture evaluates its own capabilities during compilation. If a pipeline requires a parser, a driver, or a buffer, the engine validates its presence before generating a single line of assembly, turning structural errors into build-time halts.

### 3. Absolute Determinism for Safety-Critical Systems

Because execution paths and memory boundaries are declared as static invariants, your software's behavior is 100% reproducible and predictable. You gain the high-level expressiveness of a functional dataflow language (like Haskell) while maintaining the bare-metal performance profile of raw assembly.

---

## Architecture & Reference

* 📘 **[Component Architecture Guide](docs/COMPONENTS.md)** – Understand how `component`, `vpin`, and structural blocks isolate logic from physical hardware.
* 📕 **[API Reference Manual](docs/REFERENCE.md)** – Detailed type definitions, static methodologies, and configuration trait options.

