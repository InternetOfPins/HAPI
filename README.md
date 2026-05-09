# Happy API (HAPI)

**Build modular, zero-overhead, highly composable output and UI stacks in C++.**

A powerful **static composition engine** for embedded systems, Arduino, and modern C++.

---

## Why Happy API?

Most embedded UI libraries lock you into rigid hierarchies or expensive virtual calls.  
**Happy API** lets you compose features like Lego blocks — at compile time — with full type safety and minimal overhead.

---

## Features

- Fully static composition using CRTP + Mixins
- Powerful feature chaining and reordering
- Dependency validation (`Requires` / `Excludes`)
- Marker-based introspection (`Has<...>`)
- Optional virtual interface layer
- Designed for performance-critical and resource-constrained environments

---

## Quick Start

```cpp
using MyOutput = OutDef<
    Gate<>,           // locking, measuring, partial updates
    UTF8<>, 
    TextWrap<>,
    Cursor<>,
    MyRenderer<>      // your hardware driver
>;

MyOutput out;
out.put("Hello from Happy API! 🎉");
```

---

## Technical Overview

Happy API is built around a **custom static composition engine** based on CRTP (Curiously Recurring Template Pattern) and chained mixins.

### Core Concepts

- **`Chain<>`** — The heart of the composition system. It builds a linear inheritance chain from the features you provide.
- **`APIOf<Base, Feature1, Feature2...>`** — The main composition template.
- **`OutDef<Features...>`** — User-friendly alias that creates a complete output stack.
- **`Part<T>`** — Every feature implements a `template<typename T> struct Part` that receives the next type in the chain.

### Feature Anatomy

```cpp
struct Gate {
    template<typename O>
    struct Part : O {
        using Base = O;
        using HasGate = std::true_type;

        void put(auto o) {
            if (unlocked()) Base::put(o);
        }
    };
};
```

### Key Mechanisms

| Mechanism          | Purpose                              | Example |
|--------------------|--------------------------------------|--------|
| `Requires<>`       | Declare dependencies                 | `Cursor` requires `DataParser` |
| `Excludes<>`       | Declare incompatibilities            | Mutual exclusions |
| `Has<Tag>`         | Query composition at compile time    | `Has<HasGate>` |
| `Ins<>` / `App<>`  | Dynamic reordering                   | Insert features anywhere |

### Performance Characteristics

Happy API is designed with **performance and code size as first-class citizens**:

- **Zero runtime overhead** on the hot path when using the pure static version (`OutDef<...>`). All calls are resolved at compile time via static dispatch (inlining-friendly).
- **Very small binary size** — no vtables unless you explicitly use the virtual facade (`IOutDef<...>`).
- **Excellent inlining potential** — the compiler sees the entire composed stack.
- **Conditional features** (like `Gate`) add minimal cost because checks are usually compile-time constant or easily optimized away.
- **Scalability notes**:
  - 4–8 features: Excellent compile times and code size.
  - 10–15 features: Still very good on modern compilers.
  - 20+ complex features: Compile times and debug symbols can grow significantly (typical for heavy template metaprogramming).

**Comparison**:
- Much faster and smaller than traditional virtual inheritance or runtime plugin systems.
- Comparable (or better) than hand-written monolithic classes, while being far more flexible.

**Pro Tip**: Use the static `OutDef<...>` for performance-critical code. Use `IOutDef<...>` only when you need to store different output types in containers or arrays.

### Static Validation

All dependency rules are enforced at compile time.

---

## Philosophy

> "Make the common case **blazing fast** and the advanced case **still possible and clean**."

Happy API was created as the next evolution of [ArduinoMenu](https://github.com/neu-rah/ArduinoMenu).

---

## Current Status

Early but functional stage. Focused on correctness, performance, and extensibility.

---

## Contributing

Highly welcome! Especially:
- New hardware drivers
- Useful features
- Improving compile-time diagnostics and error messages

---

**Made with obsession in the Azores** 🇵🇹  
By [Rui Azevedo](https://github.com/neu-rah) • [@ruihfazevedo](https://x.com/ruihfazevedo)

