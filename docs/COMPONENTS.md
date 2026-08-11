# HAPI Components

## Component Anatomy

A HAPI component has two distinct parts: the **layer struct** (the feature definition) and optionally a **composition wrapper** (the user-facing composed type).

---

### The layer struct

```cpp
struct A {                              // (1) outer struct — the layer identity, never instantiated directly

  template<typename O>                  // (2) O is the composed type of everything below in the stack
  struct Part : O {                     // (3) the actual mixin — single-inheritance from O
    using Base = O;                     // (4) alias for readability
    using Base::Base;                   // (5) forward constructors

    // not a component requirement — just an example of override and chain call
    template<typename Out>
    void print(Out& out) {              // (6) override a method
      out << "/A";                      //     add behavior
      Base::print(out);                 // (7) forward to the layer below — always, unless suppressing
    }
  }; // Part end

  template<typename Before, typename After>
  static constexpr bool rules() {      // (8) optional — validate ordering at composition time
    static_assert(Requires<TagIs<aTag>, Before>, "A requires a tagged component before it");
    static_assert(Excludes<SameAs<A>,   Before>, "A must not appear twice");
    return true;                        // (9) required return value
  }
};
```

| # | Element | Required | Description |
|---|---|---|---|
| 1 | `struct A` | yes | The layer identity. Never instantiated directly. |
| 2 | `template<typename O>` | yes | `O` is the composed type of everything below this layer — `Chain` resolves every link as `O::template Part<...>`; without this template there's nothing to resolve into. |
| 3 | `struct Part : O` | yes | The actual mixin — single-inheritance from `O` (mono_block topology). Together with (2), this pair *is* the composition mechanism; every layer needs it, even one that adds no behavior beyond forwarding. |
| 4 | `using Base = O` | no | Convenience alias, used to call through |
| 5 | `using Base::Base` | yes* | Essential for compound construction — propagates constructors from `O` up through every layer so the composed type can be instantiated with arguments. Without it the chain breaks at this level. |
| 6 | Method override | no | Add or transform behavior at this level |
| 7 | `Base::method()` | no* | Forward to the layer below — omit only when intentionally suppressing |
| 8 | `rules<Before,After>()` | no | Declare ordering constraints — detected automatically by `HasRules` |
| 9 | `return true` | yes* | Required when `rules()` is declared |

#### Tagging: outer struct vs inner alias

Two tagging styles exist. **Prefer outer struct inheritance** — it is visible to `query<>` and `rules<>` without instantiating `Part`:

```cpp
// Preferred — outer struct inherits the tag; query<TagIs<aFormat>, Chain<...>> works
struct aFormat {};
struct MyFmt : aFormat {
  template<typename O>
  struct Part : O { ... };
};

// Older style — tag is a type alias inside Part; only visible after Part is instantiated
struct MyFmt {
  template<typename O>
  struct Part : O { using IsFormat = std::true_type; };
};
```

#### Rules helpers

`Requires<P, Chain>` and `Excludes<P, Chain>` are convenience wrappers over `query<>`:

```cpp
template<typename Before, typename After>
static constexpr bool rules() {
  static_assert(Requires<TagIs<aFormat>,    After>,  "format layer required below");
  static_assert(Excludes<SameAs<MyLayer>,   Before>, "MyLayer must not appear before this");
  static_assert(Excludes<SameAs<MyLayer>,   After>,  "MyLayer must appear only once");
  return true;
}
```

---

### The composition wrapper

A wrapper closes the chain into a named, user-facing type and optionally adds convenience members.

```cpp
template<typename... OO>
struct ItemDef : APIOf<ItemAPI<>, OO...> {    // (1) inherit from APIOf — triggers composition and rule validation
  using Base = APIOf<ItemAPI<>, OO...>;       // (2) alias the base
  using Base::Base;                           // (3) forward constructors
  static constexpr const size_t size          // (4) optional extra members
    {sizeof...(OO)};
};

// (5) optional: stream operator for ergonomic output
template<typename Out, typename... OO>
Out& operator<<(Out& out, const ItemDef<OO...>& o)
  { o.print(out); return out; }

// (6) optional: teach query<> to look inside the wrapper's layer list
template<typename Q, typename... OO>
constexpr const bool query<Q, ItemDef<OO...>>
  { (query<Q, OO> || ...) };
```

| # | Element | Required | Description |
|---|---|---|---|
| 1 | `APIOf<API, OO...>` | yes | Closes the chain, derives from all layers + API base |
| 2 | `using Base` | no | Convenience alias |
| 3 | `using Base::Base` | yes* | Forward constructors — propagates `APIOf`'s constructors so the composed type can be built with arguments. Omit only if the composed type is only ever default-constructed. |
| 4 | Extra members | no | `size`, helper methods, anything the composed type should expose directly |
| 5 | `operator<<` | no | Ergonomic stream output |
| 6 | `query` specialisation | no | Allows `query<Q, ItemDef<OO...>>` to search the wrapper's layer list |

---

### Full worked example

```cpp
#include <hapi/hapi.h>
using namespace hapi;

// ── API base ──────────────────────────────────────────────
template<typename Cfg = Nil>
struct ItemAPI : Cfg {
  template<typename Out>
  static constexpr void print(Out& out) { out << "/"; }  // default implementation
};

// ── Composition wrapper ───────────────────────────────────
template<typename... OO>
struct ItemDef : APIOf<ItemAPI<>, OO...> {
  using Base = APIOf<ItemAPI<>, OO...>;
  using Base::Base;
  static constexpr const size_t size{sizeof...(OO)};
};

template<typename Out, typename... OO>
Out& operator<<(Out& out, const ItemDef<OO...>& o) { o.print(out); return out; }

template<typename Q, typename... OO>
constexpr const bool query<Q, ItemDef<OO...>>{(query<Q, OO> || ...)};

// ── Layers ────────────────────────────────────────────────
struct A {
  template<typename O>
  struct Part : O {
    using Base = O;
    using Base::Base;
    template<typename Out>
    static constexpr void print(Out& out) { out << "/A"; Base::print(out); }
  };
};

struct B {
  template<typename O>
  struct Part : O {
    using Base = O;
    using Base::Base;
    template<typename Out>
    static constexpr void print(Out& out) { out << "/B"; Base::print(out); }
  };

  template<typename Before, typename After>
  static constexpr bool rules() {
    static_assert(Requires<SameAs<A>, Before>,  "B requires A before it");
    static_assert(Excludes<SameAs<B>, After>,   "B must not appear twice");
    static_assert(Excludes<SameAs<A>, After>,   "A must be placed before B");
    return true;
  }
};

// ── Usage ─────────────────────────────────────────────────
constexpr ItemDef<A, B> ok{};
// constexpr ItemDef<B>     fail_requireA{};    // error: "B requires A before it"
// constexpr ItemDef<B, A>  fail_order{};       // error: "A must be placed before B"
// constexpr ItemDef<A,B,B> fail_uniqueness{};  // error: "B must not appear twice"

// ok.print(cout)  →  /A/B/
// cout << ok      →  /A/B/
```

The `print` call chain follows the type list order: `A::Part::print` → `B::Part::print` → `ItemAPI::print`, producing `/A/B/`.

---

### Predicates and Transformations

HAPI uses a two-tier metaprogramming system to manage component discovery and structural manipulation:

- **Predicates** — define capabilities or search criteria. Structured as types with `Apply`/`Check`/`ApplyPack` members, used with `query<>` to introspect the stack.
- **Transformations** — define how the type list is processed or manipulated. `Traverse`-based templates that walk the chain to map, filter, or extract type-level information.

#### Predicate Anatomy

A predicate has three members, so `Traverse` can plug it in generically — `Apply<O>` is the actual leaf-level test; `Check<O>` is the whole-tree walk built on top of it via `Traverse`; `ApplyPack<OO...>` tells `Traverse` how to fold results back together over a `Chain`:

```cpp
template<typename Q>
struct SameAs {
  template<typename O> using Apply    = std::is_same<Q,O>;                     // the leaf test
  template<typename O> using Check    = typename Traverse<SameAs<Q>,O>::Beta;  // walks the whole tree
  template<typename... OO> using ApplyPack = Chain<OO...>;                     // how Traverse folds a Chain
};
```

`query<Q, O>` (used by `Requires`/`Excludes`) resolves to `Exists<Q,O>::value`, which folds `Apply` over the whole tree via `Any<Q>` — it does not call `Check` directly, but the effect is the same: a presence test that never fails to compile.

HAPI provides built-in logical combinators for composing predicates:

```cpp
query<Not<SameAs<A>>, Chain<...>>            // negation
query<And<SameAs<A>, SameAs<B>>, Chain<...>> // conjunction
query<Or<SameAs<A>, SameAs<B>>, Chain<...>>  // disjunction
```

Predicates are always used as bare types — `SameAs<Q>`, never `SameAs<Q>{}` — passed as template arguments to `query<>`, `find<>`, `FindFirst<>`, etc. There is no runtime predicate-instance or tag-dispatch form in the core API.

#### Transformation Anatomy

Transformations plug into the same `Traverse<Op,Input>` recursion point predicates use — `Traverse` calls `Op::Apply<Input>` on a leaf, or folds `Op::ApplyPack<...>` over a `Chain<OO...>`'s already-transformed elements, recursing structurally into any nested `Chain`:

```cpp
// hapi::Map<F> — F is a template-template parameter (must expose ::Type), not an object
template<template<typename> class F>
struct Map {
  template<typename O> using Apply    = typename F<O>::Type;              // leaf: unwrap F<O>::Type
  template<typename O> using Check    = typename Traverse<Map<F>,O>::Beta;
  template<typename... OO> using ApplyPack = Chain<OO...>;                 // rebuild the Chain shape
};

// usual way to invoke it:
template<template<typename> class F, typename Input>
using Transform = Eval<Map<F>, Input>;
```

`Partition<Q,L=Left,R=Right>` wraps every element: `Q`-matches become `L<O>` (default `Left<O>`), non-matches become `R<O>` (default `Right<O>`). `Filter<Q>` keeps only the matching elements as-is (no wrapping), splicing any nested `Chain`'s matches back into one flat result.

> `Chain<O,OO...>` also has its own member alias `Map<M>` (`chain.h`) — a *shallow*, single-level `Chain<M<O>, M<OO>...>` that applies `M<O>` directly (no `::Type` unwrap, no recursion into nested `Chain`s). It's an older, unrelated mechanism to `hapi::Map<F>` above — same name, different shape. If you need structural recursion into nested chains, reach for `hapi::Map`/`Transform`, not `Chain::Map`.

#### Why this separation?

**Logic (`Part`)** remains distinct from **Meta-Logic (Predicates/Transformations)**. Predicates allow the compiler to *see* your composition constraints. Transformations allow the compiler to *rewrite* the chain for optimisation or code generation. The component surface stays small while enabling complex multi-layer validation and structural manipulation.

---

### Sub-chain as component

A `Chain<OO...>` can be used directly as a component inside another chain — its own `Part<T>` is defined on the struct, so no extra wrapping is needed:

```cpp
using Inner = Chain<Data, Store>;
using Outer = APIOf<API, Inner, Other>;   // Inner is expanded inline
```

To create a restricted view (hide or delete methods from a sub-chain), write an explicit wrapper struct:

```cpp
struct ReadOnly {
  template<typename O>
  struct Part : Chain<Data, Store>::template Part<O> {
    using Base = typename Chain<Data, Store>::template Part<O>;
    using Base::Base;
    void set(auto) = delete;   // hide the write path
  };
};
```

### Introspection from a node

HAPI provides free-function introspection — member `find`/`query` forms are not part of the core:

```cpp
// compile-time predicate: true if Q matches anywhere in node's chain
query<SameAs<WrapNav>, MyNodeType>

// verify Q exists in node's chain, then return node itself unchanged
hapi::find<SameAs<MyLayer>>(node)
```

`find<Q>(C& c)` is **not** extraction — it's a compile-time existence gate attached to a pass-through reference. `static_assert(HasResult<FindFirst_<Q, typename C::Types>>::value, ...)` hard-fails compilation with a named error if `Q` doesn't match anything in `C::Types`; on success it just returns `c` itself (by reference, same const-ness as the argument). There is no drill-down to a matched sub-object and no `forEach`/visitor form in HAPI core — a composition, not a component, is what `find` surfaces.

---

### Hardware Components

Hardware-bound layers provide direct physical access to system resources at the base of the chain, enabling platform-agnostic composition — swap the hardware layer to retarget the entire stack.

```cpp
template<uintptr_t Addr>
struct HardwarePart {
  using IsPeripheral = std::true_type;

  template<typename O>
  struct Part : O {
    using Base = O;
    using Base::Base;

    void write(uint8_t val) {
      // reinterpret_cast is intentional: Addr is a compile-time MMIO address.
      // The compiler inlines it as an absolute register write with no runtime indirection.
      *reinterpret_cast<volatile uint8_t*>(Addr) = val;
    }
  };
};
```

The address `Addr` is a compile-time template parameter — the compiler embeds it as an immediate in the instruction stream, maintaining the zero-overhead guarantee while providing a standard layer interface for peripheral access.

> **On component isolation:** keeping data members `private` in `Part` confines them absolutely to the layer that owns them — guaranteed by standard C++ access control, not a discipline. `protected` and `public` members remain reachable by layers above, by declared choice; that choice, not its enforcement, is what's left to discipline.

---

*Part of the [InternetOfPins](https://github.com/InternetOfPins) project family.*  
*Author: Rui Azevedo (neu-rah) · Azores, Portugal*
