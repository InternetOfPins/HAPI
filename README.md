# Happy API (HAPI)

**A lightweight, zero-overhead, generic static composition engine for C++.**

Build modular, type-safe, and highly composable APIs using advanced CRTP + mixin chains with compile-time validation.

---

## Why HAPI?

Most C++ codebases end up with rigid class hierarchies or expensive runtime polymorphism.  
**HAPI** offers a different approach: true **Lego-style composition at compile time**.

It gives you maximum flexibility while maintaining near-zero runtime cost and excellent compatibility with constrained environments (including ATTiny13).

---

## Features

- Very compact `Chain` + `APIOf` composition system
- Support for partial/reusable compositions
- Powerful per-feature compile-time validation (`Rules<>`)
- Introspection with `Has<>` / `Lacks<>`
- Classic `Requires` / `Excludes` support
- Clean core engine — not tied to any specific domain
- Easy to add virtual facades when dynamic polymorphism is needed

---

## Quick Start

```cpp
// Reusable partial composition
using BaseParsers = Chain<UTF8, TextWrap, Clip>;

// Final composition
using MyStack = OutDef<
  BaseParsers,
  Gate,
  MyCustomDevice
>;

MyStack s;
s.put("Hello from HAPI!");
```

HAPI can be used for **any** kind of API: output layers, menu systems, data pipelines, state machines, drivers, etc.

---

## Philosophy

- Make invalid states unpresentable
- Keep it compact and minimal
- Zero runtime overhead by default
- Compile-time safety where it matters

---

## Current Status

**Active RnD / Early Stage**  
Core engine is functional. Examples are working.  
Still evolving quickly.

---

**Made with obsession in the Azores** 🇵🇹  
By [Rui Azevedo](https://github.com/neu-rah) • [@ruihfazevedo](https://x.com/ruihfazevedo)
