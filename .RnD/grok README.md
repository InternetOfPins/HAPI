# HAPI - Happy API

**Zero-overhead modular composition engine for embedded C++.**

HAPI lets you build clean, highly composable, type-safe stacks (output, menus, drivers, protocols) using static CRTP mixin composition with virtually no runtime cost.

---

## Philosophy

> "Make the common case blazing fast and the advanced case still clean and possible."

No virtual calls on hot paths. Full compile-time safety and introspection. Designed for constrained devices (AVR, ESP, ARM Cortex-M).

---

## Current Status (May 2026)

- Version: **0.3.x** (actively evolving)
- Core engine stable
- Strong integration with **OneData** and **OneList**

---

## Key Features

- `Chain<>` — Core linear CRTP mixin engine
- `APIOf<Base, Features...>` — Main composition template
- `OutDef<Features...>` — Convenient output stacks
- Rules system: `Requires<>`, `Excludes<>`
- Introspection: `Has<>`
- Reordering: `Ins<>`, `App<>`, `Join<>`

---

## Usage Examples

### 1. Simple Output Stack

```cpp
using MyOut = OutDef<
    Gate<>,
    UTF8<>,
    TextWrap<>,
    Cursor<>,
    ANSIOut<>,
    ConsoleOut
>;

MyOut out;
out.put("Hello from HAPI!");
```

### 2. Advanced Menu Rendering Pipeline

```cpp
using ScrollPrinter = Chain<
    ViewPrinter,
    MenuPrinter<
        TitlePrinter,
        ScrollBodyPrinter,
        ItemPrinter<IndexPrinter, NavCursorPrinter, ItemBodyPrinter>
    >
>;

using MainOutput = OutDef<
    ScrollPrinter,
    ANSIFmt<>,
    ClearFreeFmt<>,
    DataParser<>,
    UTF8<>,
    Clip<>,
    ANSIOut<>,
    ConsoleOut
>;
```

### 3. Data Components

```cpp
using Volume = Chain<
    DataPrint,
    Data<int>,
    Watch<>,
    NumRange<int>
>;

Volume volume{75};
volume.up(10);
if (volume.changed()) { /* react */ }
```

### 4. Menu Item

```cpp
auto ledItem = ItemDef<
    Id<"led">,
    Text,
    Action<toggleLed>
>("Toggle LED");
```

---

## Ecosystem

- **HAPI** — Core composition engine
- **OneData** — Data components
- **OneList** — Heterogeneous lists

---

## Links

- Repository: https://github.com/InternetOfPins/HAPI

---

**Author**: Rui Azevedo (neu-rah)  
**Contributor**: Grok (xAI) - architecture, cleanup & modern C++ patterns
