# HAPI — Happy API

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**A zero-overhead static composition engine for C++.**

Components compose into **type trees**. The compiler flattens them to optimal machine code. The structure never reaches runtime.

---

```cpp
#include <hapi/hapi.h>
using namespace hapi;

// Each component declares open-ended inheritance
struct Validate { template<typename O> struct Part : O { using O::O; }; };
struct Log      { template<typename O> struct Part : O { using O::O; }; };
struct Cache    { template<typename O> struct Part : O { using O::O; }; };

// Flat chain
using Node = APIOf<DriverAPI, Validate, Log, Cache>;

// Or a tree — nested chains, natively traversable
using Inner = Chain<Validate, Log>;
using Tree  = APIOf<DriverAPI, Inner, Cache>;

// Map a transform over any topology — tree structure preserved
using Mapped = Map<MakePointer, Tree>::Expr;
```

No virtual dispatch. No heap allocation. Wrong layer order → **named compile error**.

---

## The Central Idea

Most heterogeneous composition libraries operate on **flat type lists**. HAPI operates on **type trees**.

A nested `Chain<Branch<A,B>, Branch<C,D>>` is natively traversable — `Map`, `FindFirst`, `forEach` all preserve and traverse the tree topology without collapsing it. Libraries built on flat lists lose tree topology internally: operations see a flat sequence regardless of original structure, with no way to recover the nesting.

This matters when structure *is* the semantics: layered protocols, nested menus, device hierarchies, validation pipelines.

---

## Open Chain Derivation

The central mechanism. Each component declares an inner `Part<O>` template that inherits from `O`. A chain folds these into a single C++ class through recursive inheritance — the **base is provided by the caller**, not fixed by the chain.

```
Chain<A, B, C>::Part<API>  ≡  A::Part<B::Part<C::Part<API>>>
```

The compiler sees the full resolved hierarchy and flattens it. Composed fields are packed into a single contiguous memory block, sized exactly to what was declared. No heap, no fragmentation.

---

## Core Pillars

- **Type Tree Composition** — nested chains are first-class: mappable, filterable, queryable without losing structure.
- **Zero Runtime Cost** — no vtables, no dynamic allocation. All abstraction paid at compile time.
- **Type-Level Validation** — structural and semantic rules verified at compilation. Invalid compositions don't compile.
- **Functional Operations** — `Map<F, Tree>`, `FindFirst<Q>`, `forEach<Q>`, `FilterIf<Q>` — all topology-aware.
- **Value Transforms** — `At<N,T>` for indexed storage, `Mapped<F>` and `Trans<F>` for zero-storage constexpr pipelines.

HAPI runs anywhere C++17 runs — AVR, ESP32, Linux, bare metal.

---

## HAPI and Boost.Hana

Complementary, not competing. Hana excels at value-level heterogeneous computation over flat sequences of type-encoded constants (`int_c`). HAPI excels at structural composition over arbitrary tree topologies.

| | HAPI | Hana |
|---|---|---|
| Domain | Type trees, structural composition | Value sequences, integral constants |
| Tree topology | Native — Map/Find preserve structure | Must flatten first |
| Value computation | Delegates to Hana | Native |
| Target | Embedded + systems, any C++17 | General C++ |

The boundary is clean: HAPI owns the structure; Hana owns the value-level algebra.

---

## Runtime Companion (`run.h`)

Composable runtime predicates and constexpr transform pipelines, same chain architecture:

```cpp
// Predicate fold: Equal AND Greater, terminal = True
using Filter = APIOf<run::True, run::Equal<int>, run::Greater<int>>;

// Constexpr transform pipeline: composed bottom-up, zero storage
struct Inc { constexpr int operator()(int n) const { return n + 1; } };
struct Dbl { constexpr int operator()(int n) const { return n * 2; } };
using Pipeline = APIOf<run::Identity, run::Trans<Dbl>, run::Trans<Inc>>;
// Pipeline::transform(3) == Dbl(Inc(3)) == 8
```

---

## The Win-Win Architecture

- **The developer wins** — expressive, modular, reusable code. Composition is declared, not wired.
- **The hardware wins** — flat, optimal instruction sequences. No vtables, no dynamic allocation, no indirection.
- **The compiler pays the price** — all abstraction cost is paid in build-time seconds. The final binary contains none of it.

There is no such thing as a structurally broken HAPI program that compiles.

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

*Made with obsession in the Azores* 🇵🇹  
By [Rui Azevedo](https://github.com/neu-rah) · [@ruihfazevedo](https://x.com/ruihfazevedo)
