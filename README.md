# HAPI — Happy API

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**A zero-overhead static composition engine for C++.**

Enables modular, type-safe, and high-performance system design through advanced compile-time composition.

---

```cpp
#include <hapi/hapi.h>
using namespace hapi;

OutDef<
  ScrollPrinter, ANSIFmt, ClearFreeFmt,
  DataParser<>, CtrlChars, UTF8, TextWrap, Clip,
  ColorTrack<int>, Cursor, Gate, ANSIOut,
  ConsoleOut, StaticPos<20,10>, StaticArea<30,8>
> out;
```

No virtual dispatch. No heap allocation. Wrong layer order → **named compile error**.  
The same pipeline compiles to AVR UART or POSIX stdout by swapping one layer.

---

## Why HAPI?

Modern embedded and systems programming often forces a choice between clean architecture and runtime performance. HAPI provides a third path: **zero-overhead static composition**. Developers write modular, composable, high-level code — the compiler transforms it into flat, optimal machine instructions.

---

## The Win-Win Architecture

- **The developer wins** — expressive, modular, reusable code. Composition is declared, not wired.
- **The hardware wins** — flat, optimal instruction sequences. No vtables, no dynamic allocation, no indirection.
- **The compiler pays the price** — all abstraction cost is paid in build-time seconds. The final binary contains none of it.

There is no such thing as a structurally broken HAPI program that compiles.

---

## Open Chain Derivation

The central mechanism of HAPI. An ordered list of feature types is folded into a single C++ class through recursive open-ended inheritance. Each feature contributes an inner `Part<O>` template inheriting from `O` — the layer below. The base is provided by the caller, not fixed by the chain.

The compiler sees the full resolved hierarchy and flattens it. The result is indistinguishable in performance from a hand-written monolith — because structurally, it is one.

---

## Core Pillars

- **Static Composition** — assembled at compile time into flat, cache-friendly structures with full inlining.
- **Type-Level Validation** — structural and semantic rules verified at compilation.
- **Zero Runtime Cost** — no vtables, no dynamic allocation, predictable memory layout.
- **Functional Influence** — composition, immutability of structure, making invalid states irrepresentable.

HAPI runs anywhere C++17 runs.

---



## Documentation

- **[Industry Applications](docs/INDUSTRY.md)** — Where the pattern applies and why it matters.
- **[Component Architecture](docs/COMPONENTS.md)** — Component anatomy, layer structure, worked examples.
- **[API Reference](docs/REFERENCE.md)** — Core types and advanced usage.

---

## Related Projects

| Project | Description |
|---|---|
| [ArduinoMenu v5 (AM5)](https://github.com/neu-rah/AM5) | Full TUI menu framework built on HAPI |
| [ArduinoMenu v4](https://github.com/neu-rah/ArduinoMenu) | Previous generation — 1k★, 197 forks, GitHub Arctic Code Vault |
| [OneList](https://github.com/InternetOfPins/OneList) | Heterogeneous runtime list with compile-time type mirror |
| [streamFlow](https://github.com/neu-rah/streamFlow) | Lightweight `<<` stream operator for Arduino |

---

**Made with obsession in the Azores** 🇵🇹  
By [Rui Azevedo](https://github.com/neu-rah) · [@ruihfazevedo](https://x.com/ruihfazevedo)