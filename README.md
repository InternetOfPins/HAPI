# <img src="logo.png" alt="HAPI logo" width="32" height="32"> HAPI — Happy API

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/neu-rah/library/HAPI.svg)](https://registry.platformio.org/libraries/neu-rah/HAPI)
<a href="https://www.paypal.me/ruihfazevedo" rel="nofollow"><img src="https://camo.githubusercontent.com/604e3db9c8751116b3f765aad0353ec7ded655bbe8aaacbc38d8c4a6b784b3ed/68747470733a2f2f696d672e736869656c64732e696f2f62616467652f446f6e6174652d50617950616c2d677265656e2e737667" alt="Donate" data-canonical-src="https://img.shields.io/badge/Donate-PayPal-green.svg" style="max-width: 100%;"></a>

**A zero-overhead static composition engine for C++.**

Components compose into **type trees**. The compiler flattens them to optimal machine code. The structure never reaches runtime.

---

```cpp
#include <hapi/hapi.h>
using namespace hapi;

// Each component adds one behavior to whatever it's given
struct Clamp  { template<typename O> struct Part : O {
  using O::O;
  int read() { int v = O::read(); return v < 0 ? 0 : v > 100 ? 100 : v; }
}; };

struct Smooth { template<typename O> struct Part : O {
  using O::O;
  int last = 0;
  int read() { int v = O::read(); last = (last + v) / 2; return last; }
}; };

struct Log    { template<typename O> struct Part : O {
  using O::O;
  int read() { int v = O::read(); Serial.println(v); return v; }
}; };

// Compose: raw sensor -> clamp -> smooth -> log
using Sensor = APIOf<AnalogPin<A0>, Clamp, Smooth, Log>;

Sensor sensor;
sensor.read();   // prints one clamped, smoothed reading — no vtable involved
```

No virtual dispatch. No heap allocation. Wrong layer order → **named compile error**.

---

## The Central Idea

Most heterogeneous composition libraries operate on **flat type lists**. HAPI operates on **type trees**.

A nested `Chain<Branch<A,B>, Branch<C,D>>` is natively traversable — `Map`, `FindFirst` all preserve and traverse the tree topology without collapsing it. Libraries built on flat lists lose tree topology internally: operations see a flat sequence regardless of original structure, with no way to recover the nesting.

This matters when structure *is* the semantics: layered protocols, nested menus, device hierarchies, validation pipelines.

The sensor example above is a flat chain. Nesting works the same way — a sub-chain is just another component:

```cpp
using Filtering = Chain<Clamp, Smooth>;      // reusable sub-chain
using Sensor    = APIOf<AnalogPin<A0>, Filtering, Log>;
```

`Filtering` can be reused across multiple sensors, tested on its own, or swapped out — and `Map`/`FindFirst` still see and traverse into it as a nested branch, not a flattened blob.

---

## Open Chain Derivation

The mechanism behind the example above. Each component declares an inner `Part<O>` template that inherits from `O`. A chain folds these into a single C++ class through recursive inheritance — the **base is provided by the caller**, not fixed by the chain.

```
Chain<Clamp, Smooth, Log>::Part<AnalogPin<A0>>
  ≡  Clamp::Part<Smooth::Part<Log::Part<AnalogPin<A0>>>>
```

The compiler sees the full resolved hierarchy and flattens it. Composed fields are packed into a single contiguous memory block, sized exactly to what was declared. No heap, no fragmentation.

---

## Core Pillars

- **Type Tree Composition** — nested chains are first-class: mappable, filterable, queryable without losing structure.
- **Zero Runtime Cost** — no vtables, no dynamic allocation. All abstraction paid at compile time.
- **Type-Level Validation** — structural and semantic rules verified at compilation. Invalid compositions don't compile.
- **Topology-Preserving Operations** — `Map<F>`, `FindFirst<Q>`, `Filter<Q>` traverse any tree shape natively.
- **Query Machinery** — `SameAs<T>`, `IsInstanceOf<W>`, `FromTypes<Q>` for introspection; `And<A,B>`, `Or<A,B>`, `Not<Q>` for composition.
- **Soft-Fail Variants** — `Exists<Q>` (bool), `FindFirstOr<Q, Default>` for non-fatal searches.
- **Runtime References** — `find<Q>(object)` / `findOr<Q, Default>(object)` — compile-time query, runtime ref into composed object.

HAPI runs anywhere C++17 runs — AVR, ESP32, Linux, bare metal. Its compile-time composition layer also synthesizes to hardware, verified against [PandA-Bambu](https://github.com/ferrandi/PandA-bambu) HLS — see [Industry Applications](docs/INDUSTRY.md#fpga--cpld-register-interfacing).

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

## Type-Level Queries & Runtime Resolution

Predicates compose freely — check structure at compile time, resolve to runtime references:

```cpp
// Hard-fail query: find the Log stage inside a composed sensor
using Pred = IsInstanceOf<Log>;
auto& logger = find<Pred>(sensor);  // Compile-time walk, runtime reference

// Soft-fail variant: check presence without error
if constexpr (Exists<Pred, Sensor>::value) {
  // Only instantiate this if the query can succeed
}
```

The compiler verifies the query is satisfiable; the code gets a typed reference into the runtime object graph.

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
| [ArduinoMenu v4](https://github.com/neu-rah/ArduinoMenu) | Previous generation — 1k★, 197 forks, GitHub Arctic Code Vault |
| [streamFlow](https://github.com/neu-rah/streamFlow) | Lightweight `<<` stream operator for Arduino |

---

*Made with obsession in the Azores* 🇵🇹
By [Rui Azevedo](https://github.com/neu-rah) · [@ruihfazevedo](https://x.com/ruihfazevedo)
