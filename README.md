# HAPI

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

## Core Pillars

- **Static Composition** — Components are assembled at compile time into flat, cache-friendly structures with full inlining.
- **Type-Level Validation** — Structural and semantic rules are verified during compilation using powerful predicates.
- **Zero Runtime Cost** — All composition and validation happens statically. No vtables, minimal indirection, predictable memory layout.
- **Functional Influence** — Strong emphasis on composition, immutability of structure, and making invalid states unpresentable.

---

## Documentation

* **[Component Architecture](docs/COMPONENTS.md)** — How components, structural blocks, and hardware abstraction layers work.
* **[API Reference](docs/REFERENCE.md)** — Core types, patterns, and advanced usage.

---

**Made with obsession in the Azores** 🇵🇹  
By [Rui Azevedo](https://github.com/neu-rah) • [@ruihfazevedo](https://x.com/ruihfazevedo)

