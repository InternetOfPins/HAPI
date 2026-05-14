# Happy API (HAPI)

**Modular zero-overhead static composition engine for C++.**

Build highly composable, type-safe APIs with CRTP + mixins, focused on embedded systems and extreme resource constraints.

---

## Why HAPI?

Tired of rigid hierarchies and virtual call overhead?  
HAPI lets you compose features like Lego at compile time — with full static dispatch and excellent AVR support (ATTiny13+).

---

## Features

- Extremely lightweight static composition (`Chain` + `APIOf`)
- `Requires` / `Excludes` + new `Rules<>` system
- `Has<>` / `Lacks<>` introspection
- Partial compositions (`Chain<...>`) for reuse
- Dual support: `OutDef<>` and `ItemDef<>`
- Optional virtual facades (`IOut` / `IItem`)

---

## Quick Start

```cpp
using MyOut = OutDef<
  UTF8,
  TextWrap,
  Cursor,
  Gate,
  MyDevice
>;

MyOut out;
out.put("Hello HAPI");