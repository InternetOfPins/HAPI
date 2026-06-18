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
| 2 | `template<typename O>` | no | `O` is the composed type of everything below this layer. Omit only when the component adds no behavior. |
| 3 | `struct Part : O` | no | The actual mixin — single-inheritance from `O` (mono_block topology). |
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
| 3 | `using Base::Base` | yes | Forward constructors |
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

- **Predicates** — define capabilities or search criteria. Structured as type-traits returning a boolean `value`, used with `query<>` to introspect the stack.
- **Transformations** — define how the type list is processed or manipulated. Recursive templates that walk the chain to map, filter, or extract type-level information.

#### Predicate Anatomy

```cpp
template<typename Q>
struct SameAs {
  template<typename O> struct Check {
    static constexpr const bool value{std::is_same_v<O, Q>};
  };
};
```

HAPI provides built-in logical combinators for composing predicates:

```cpp
query<Not<SameAs<A>>, Chain<...>>            // negation
query<And<SameAs<A>, SameAs<B>>, Chain<...>> // conjunction
query<Or<SameAs<A>, SameAs<B>>, Chain<...>>  // disjunction
```

#### Variable-template shorthands

Predicates are empty types; constructing them with `{}` can be noisy. Downstream libraries define their own variable templates for frequent predicates (e.g. `byId<V>` in OneItem). HAPI itself does not ship any — `SameAs<Q>{}` is the baseline:

```cpp
node.find(SameAs<MyLayer>{})           // tag-dispatch — no .template needed
node.template find<SameAs<MyLayer>>()  // explicit template arg — needs .template in template context
```

#### `is_predicate<Q>`

```cpp
template<typename Q, typename = void> struct is_predicate : std::false_type {};
template<typename Q>
struct is_predicate<Q, std::void_t<decltype(Q::template Check<void>::value)>>
  : std::true_type {};
```

Detects valid predicates. Used by `find(Q)` and `query(Q)` tag-dispatch overloads to give a clear `static_assert` error rather than a template instantiation wall when a non-predicate type is accidentally passed.

#### Transformation Anatomy

Transformations provide a recursive interface to walk and rewrite the stack:

```cpp
// Map a transformation F over a single type
template<typename F, typename O>
struct Map {
  using Expr = typename F::template Apply<O>::Expr;
};

// Map a transformation F over every type in a Chain
template<typename F, typename... OO>
struct Map<F, Chain<OO...>> {
  using Expr = Chain<typename Map<F, OO>::Expr...>;
};
```

`Partition<Q>` categorises each type as `Right<T>` (matches) or `Left<T>` (doesn't). `FilterIf<P, Chain<...>>` extracts matching types, unwrapping their inner type in the process.

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

// locate the first matching component and return a reference
hapi::find<SameAs<MyLayer>>(node)

// visit all matching components
hapi::forEach<TagIs<aWrapNav>>(node, [](auto& part) { part.enable(false); });
```

**Non-predicate guard:** `find<Q>(node)` fires `is_predicate<Q>` `static_assert` if `Q` lacks a `Check` member, producing a human-readable error at the call site.

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

> **On component isolation:** keeping data members `private` in `Part` limits their visibility to the layer that owns them. However, because layers compose through inheritance, `protected` and `public` members remain accessible to layers above. Full blast-radius isolation between layers is not achievable through this pattern alone — isolation is a design discipline, not a structural guarantee.

---

*Part of the [InternetOfPins](https://github.com/InternetOfPins) project family.*  
*Author: Rui Azevedo (neu-rah) · Azores, Portugal*
