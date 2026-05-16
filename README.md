# Happy API (HAPI)

**Build modular, zero-overhead, highly composable output and UI stacks in C++.**

A powerful **static composition engine** for embedded systems, Arduino, and modern C++.

---

## Why Happy API?

Most embedded UI/output libraries force rigid hierarchies or expensive virtual calls.  
**HAPI** lets you compose features like Lego blocks — **at compile time** — with full type safety and virtually zero overhead.

---

## Features

- Fully static CRTP mixin composition
- Powerful chaining + reordering (`Ins<>`, `App<>`, `Join<>`)
- Strong dependency validation (`Requires<>`, `Excludes<>`)
- Rich compile-time introspection (`Has<>`)
- Optional virtual facade (`IOutDef<>`)
- Designed from the ground up for tiny devices (AVR, ESP, etc.)

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

## Core Concepts

- **`Chain<>`** — Linear CRTP mixin chain (the engine heart)
- **`APIOf<Base, Features...>`** — Main composition template
- **`OutDef<Features...>`** — Convenient alias for output stacks
- **`Part<O>`** — Every feature provides a `template<typename O> struct Part : O`

### Feature Example

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

---

## Key Mechanisms

| Mechanism       | Purpose                        | Example                     |
|-----------------|--------------------------------|-----------------------------|
| `Requires<>`    | Dependencies                   | `Cursor` needs `DataParser` |
| `Excludes<>`    | Incompatibilities              | Mutual exclusions           |
| `Has<Tag>`      | Introspection                  | `Has<HasGate>`              |
| `Ins<>/App<>`   | Reordering                     | Insert features anywhere    |

---

## Performance

- **Zero runtime overhead** on hot path (pure static `OutDef`)
- Excellent inlining
- Minimal code size (no vtables unless using virtual facade)
- Scales well up to ~10-12 features on embedded compilers

**Pro Tip**: Use static `OutDef<...>` for performance-critical code. Use `IOutDef<...>` only when you need polymorphism.

---

## Philosophy

> "Make the common case **blazing fast**  
> and the advanced case **still possible and clean**."

HAPI is the next evolution of the ArduinoMenu lineage.

---

## Current Status

Early but functional. Focused on correctness, performance, and tiny-device compatibility.

---

## Contributing

Highly welcome! Especially:
- New hardware drivers / renderers
- Useful features (input, protocols, menus...)
- Better compile-time diagnostics

---

**Made with obsession in the Azores** 🇵🇹  

By [Rui Azevedo](https://github.com/neu-rah) • [@ruihfazevedo](https://x.com/ruihfazevedo)
