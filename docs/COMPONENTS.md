# HAPI Components

## Component Anatomy

A HAPI component has two distinct parts: the **layer struct** (the feature definition) and optionally a **composition wrapper** (the user-facing composed type).

---

### The layer struct

```cpp
struct A {                              // (1) plain struct, no base class required
  template<typename O>                  // (2) O is the layer below in the stack
  struct Part : O {                     // (3) inherit from O — wraps the layer below
    using Base = O;                     // (4) alias for readability
    using Base::Base;                   // (5) forward constructors

    using IsA = std::true_type;         // (6) optional tag — lets other layers detect A via query

    //not a component requirement but just an example of override, and chain call
    template<typename Out>
    void print(Out& out) {              // (7) override a method
      out << "/A";                      //     add behavior
      Base::print(out);                 // (8) forward to the layer below — always, unless suppressing
    }
  };

  template<typename Before, typename After>
  static constexpr bool rules() {      // (9) optional — validate ordering at composition time
    static_assert(query<SameAs<X>, Before>, "X must come before A");
    return true;                        // (10) required return value
  }
};
```

| # | Element | Required | Description |
|---|---|---|---|
| 1 | Outer struct | yes | The layer identity. Never instantiated directly. |
| 2 | `template<typename O>` | yes | `O` is the composed type of everything below this layer |
| 3 | `struct Part : O` | yes | The actual mixin — inherits and extends `O` |
| 4 | `using Base = O` | no | Convenience alias, used to call through |
| 5 | `using Base::Base` | no | Forwards constructors from below |
| 6 | Tag alias | no | Publish presence for `query<IsA, ...>` detection |
| 7 | Method override | no | Add or transform behavior at this level |
| 8 | `Base::method()` | no* | Forward to the layer below — omit only when intentionally suppressing |
| 9 | `rules<Before,After>()` | no | Declare ordering constraints — detected automatically by `HasRules` |
| 10 | `return true` | yes* | Required when `rules()` is declared |

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
    static_assert(query<SameAs<A>, Before>, "B requires A before it");
    static_assert(!query<SameAs<B>, After>, "B must not appear twice");
    static_assert(!query<SameAs<A>, After>, "A must be placed before B");
    return true;
  }
};

// ── Usage ─────────────────────────────────────────────────
constexpr ItemDef<A, B> ok{};
// constexpr ItemDef<B>    fail_requireA{};     // error: "B requires A before it"
// constexpr ItemDef<B, A> fail_order{};        // error: "A must be placed before B"
// constexpr ItemDef<A,B,B> fail_uniqueness{};  // error: "B must not appear twice"

// ok.print(cout)  →  /A/B/
// cout << ok      →  /A/B/
```

The `print` call chain follows the type list order: `A::Part::print` → `B::Part::print` → `ItemAPI::print`, producing `/A/B/`.