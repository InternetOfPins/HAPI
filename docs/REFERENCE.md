# HAPI API Reference

> **File:** `include/hapi/hapi.h`  
> **Author:** Rui Azevedo (neu-rah) · ruihfazevedo@gmail.com  
> **Namespace:** `hapi`  
> **Brief:** A powerful modular, zero-overhead, static composition engine for embedded systems and modern C++.

---

## Contents

- [Platform & Setup](#platform--setup)
- [Core Types](#core-types)
  - [Nil](#nil)
  - [query](#query)
- [Chain](#chain)
  - [Chain\<O, OO...\>](#chaino-oo)
  - [Chain\<O,OO...\>::Part\<T\>](#part)
  - [query\<Q, Chain\<OO...\>\>](#chain-query-specialisation)
- [Predicates](#predicates)
  - [SameAs\<Q\>](#sameasq)
- [Rules System](#rules-system)
  - [HasRules\<T\>](#hasrulest)
  - [RuleLayer\<Current, Before, After, true\>](#rulelayer)
  - [BuildRules\<Before, After\>](#buildrules)
- [Composition](#composition)
  - [APIOf\<API, OO...\>](#apiofapi-oo)
  - [CRTP\<O\>](#crtpo)
- [Writing a Layer](#writing-a-layer)
- [Writing a Predicate](#writing-a-predicate)
- [Writing Rules](#writing-rules)

---

## Platform & Setup

```cpp
#include <hapi/hapi.h>
using namespace hapi;
```

On AVR, `platform/avr/avr_std.h` is included automatically in place of the standard headers. Define `HAPI_DEBUG` before including to disable the `hapi` namespace and expose `std::cout` / `std::endl` for debugging output.

---

## Core Types

### `Nil`

```cpp
struct Nil {};
```

Sentinel empty type. Use as a neutral base or default template argument where "nothing" is a valid value.

---

### `query`

```cpp
template<typename Q, typename O>
constexpr const bool query{Q::template Check<O>::value};
```

Applies predicate `Q` to type `O` at compile time.

| Parameter | Description |
|---|---|
| `Q` | Predicate type. Must expose `template<typename O> struct Check { static constexpr bool value; }` |
| `O` | Target type to test |

**Returns:** `true` if `O` satisfies `Q`, `false` otherwise.

**Usage:**
```cpp
// test a single type
constexpr bool result = query<SameAs<MyLayer>, SomeType>;

// test a chain — see Chain specialisation below
constexpr bool result = query<SameAs<MyLayer>, Chain<A, B, C>>;

// use inside a layer's rules() or static_assert
static_assert(query<SameAs<Gate>, Before>, "Gate must come before this layer");
```

---

## Chain

### `Chain<O, OO...>`

```cpp
template<typename O, typename... OO>
struct Chain<O, OO...>;
```

A compile-time ordered type list. Provides list operations and the `Part<T>` mixin fold that collapses the list into a single C++ class.

| Member | Kind | Description |
|---|---|---|
| `Types` | type alias | `Chain<O, OO...>` — the list itself, for introspection |
| `Head` | type alias | First type in the list: `O` |
| `Tail` | type alias | Remaining types: `Chain<OO...>` |
| `size` | `constexpr SizeT` | Number of types in the list |
| `App<XX...>` | alias template | Prepend `XX...` → `Chain<XX..., O, OO...>` |
| `Ins<XX...>` | alias template | Append `XX...` → `Chain<O, OO..., XX...>` |
| `Map<M>` | alias template | Apply `M<>` to each type → `Chain<M<O>, M<OO>...>` |
| `Part<T>` | struct template | Collapse the chain into a mixin inheritance hierarchy over `T` |

---

### `Chain<O,OO...>::Part<T>` {#part}

```cpp
template<typename T>
struct Part : O::template Part<typename Chain<OO...>::template Part<T>>;
```

Collapses `O, OO...` into a single C++ object by forming a recursive inheritance chain over the termination object `T`.

| Parameter | Description |
|---|---|
| `T` | Termination object — the innermost base of the inheritance chain |

**Expansion:** `Chain<A, B, C>::Part<Base>` expands to:

```
A::Part<
  B::Part<
    C::Part<Base>
  >
>
```

`A` is outermost — it receives method calls first. `Base` is innermost. Each layer wraps the one below it via its own `Part<O>` template.

**Members exposed on the result:**
- `Base` — the immediate base type (one level down)
- `Types` — `Chain<O, OO...>`, available on the composed object for introspection

---

### `query<Q, Chain<OO...>>` — Chain query specialisation {#chain-query-specialisation}

```cpp
template<typename Q, typename... OO>
constexpr const bool query<Q, Chain<OO...>>{(query<Q, OO> || ...)};
```

Folds predicate `Q` across all types in the chain. Returns `true` if **any** type in `Chain<OO...>` satisfies `Q`.

**Usage:**
```cpp
// Does this chain contain Gate?
constexpr bool has_gate = query<SameAs<Gate>, Chain<A, Gate, B>>;  // true

// Use inside rules() to inspect Before or After
static_assert(query<SameAs<DataParser>, Before>, "DataParser must precede this layer");
```

---

## Predicates

### `SameAs<Q>`

```cpp
template<typename Q>
struct SameAs {
  template<typename O> struct Check {
    static constexpr const bool value{std::is_same_v<O, Q>};
  };
};
```

Built-in predicate. Matches when the target type `O` is exactly `Q`.

| Parameter | Description |
|---|---|
| `Q` | The type to match against |

**Usage:**
```cpp
query<SameAs<MyLayer>, SomeType>          // true if SomeType == MyLayer
query<SameAs<MyLayer>, Chain<A, B, C>>    // true if any of A, B, C == MyLayer
```

Custom predicates follow the same `Check<O>::value` pattern — see [Writing a Predicate](#writing-a-predicate).

---

## Rules System

The rules system validates layer ordering at compile time. Layers opt in by declaring a `rules<Before, After>()` static method. HAPI detects its presence automatically via `HasRules` and calls it through `BuildRules` during `APIOf` instantiation.

> **Note:** `HasRules`, `RuleLayer`, and `BuildRules` are advanced API — only needed when extending HAPI itself. Normal layer authoring only requires writing `rules<Before, After>()` on the layer struct.

---

### `HasRules<T>`

```cpp
template<typename T>
struct HasRules<T, std::void_t<decltype(T::template rules<void,void>())>>
  : std::true_type {};
```

Detects whether type `T` exposes a `rules<Before, After>()` static method. No tag or marker required on the layer — detection is fully automatic via SFINAE.

| `HasRules<T>::value` | Meaning |
|---|---|
| `false` | `T` has no `rules<>()` — skipped during validation |
| `true` | `T` has `rules<>()` — called during validation |

---

### `RuleLayer<Current, Before, After, true>` {#rulelayer}

```cpp
template<typename Current, typename Before, typename After>
struct RuleLayer<Current, Before, After, true> {
  template<typename O>
  struct Part : O {
    static constexpr bool rules() {
      return Current::template rules<Before, After>() && O::rules();
    }
  };
};
```

Composes `Current`'s rules with the rules chain below it. Only instantiated when `HasRules<Current>::value` is `true`.

| Parameter | Description |
|---|---|
| `Current` | The layer whose `rules<>()` is being composed |
| `Before` | `Chain<>` of all layers that appear before `Current` in the stack |
| `After` | `Chain<>` of all layers that appear after `Current` in the stack |

---

### `BuildRules<Before, After>`

```cpp
template<typename Before, typename After>
struct BuildRules :
  RuleLayer<typename After::Head, Before, typename After::Tail>::template Part<
    BuildRules<typename Before::template App<typename After::Head>, typename After::Tail>
  >
{};
```

Walks `After` left to right, threading `Before` forward at each step. Provides each layer with the correct `Before`/`After` snapshot when its `rules<>()` is called.

| Parameter | Description |
|---|---|
| `Before` | Types already processed — starts as `Chain<>` |
| `After` | Types remaining — starts as the full layer list |

**Threading for `Chain<A, B, C>`:**

| Step | Current | Before | After |
|---|---|---|---|
| 1 | `A` | `Chain<>` | `Chain<B, C>` |
| 2 | `B` | `Chain<A>` | `Chain<C>` |
| 3 | `C` | `Chain<A, B>` | `Chain<>` |

Triggered automatically by `APIOf` — never instantiate directly.

---

## Composition

### `APIOf<API, OO...>`

```cpp
template<typename API, typename... OO>
struct APIOf : Chain<OO...>::template Part<API>;
```

Closes chain composition. Collapses `OO...` into a single C++ class that ultimately derives from `API`. Triggers `BuildRules` validation at instantiation time.

| Parameter | Description |
|---|---|
| `API` | Fallback base — the innermost base of the composed class. Provides the public interface surface. |
| `OO...` | Feature layers in stack order. First = outermost = first to receive method calls. |

**Members exposed:**
- `Base` — `Chain<OO...>::Part<API>`, the immediate base type

**Rule validation:** if any layer's `rules<Before, After>()` fires a `static_assert`, the error is reported at the `APIOf` instantiation site with the message from the assert.

**Usage:**
```cpp
struct MyAPI { /* public interface */ };

using MyDevice = APIOf<MyAPI, LayerA, LayerB, LayerC, SinkLayer>;
MyDevice dev;
dev.someMethod();
```

With `CRTP`:
```cpp
struct MyAPI : CRTP<MyAPI> { /* ... */ };

template<typename... OO>
struct MyDef : APIOf<MyAPI, OO...> {};
```

---

### `CRTP<O>`

```cpp
template<typename O>
struct CRTP {
  using Obj = O;
  O&       obj();
  const O& obj() const;
  O*       operator->();
  const O* operator->() const;
};
```

Optional. Provides circular reference from any layer back to the fully-composed object. Use when a layer deep in the stack needs to call methods defined on the outermost composed type.

| Parameter | Description |
|---|---|
| `O` | The final composed type — the `APIOf` or user-defined wrapper instantiation |

| Member | Description |
|---|---|
| `Obj` | Type alias for `O` |
| `obj()` | Downcast `*this` to `O&` |
| `operator->()` | Pointer downcast to `O*` |

> **Note:** Using `CRTP` will make compiler error messages significantly larger. It can also lead to infinite loops if `obj()` is called in a context that re-enters the same method. Use only when strictly needed.

**Usage:**
```cpp
struct MyAPI : hapi::CRTP<MyAPI> {};

// inside a layer:
template<typename O>
struct Part : O {
  void someMethod() {
    O::obj().put('x');  // calls put() on the fully-composed object
  }
};
```

---

## Writing a Layer

A layer is any struct with an inner `Part<O>` template. No base class required.

```cpp
struct MyLayer {
  // Optional: validate stack ordering at composition time
  template<typename Before, typename After>
  static constexpr bool rules() {
    static_assert(query<SameAs<RequiredLayer>, Before>,
      "MyLayer requires RequiredLayer to be placed before it in the stack");
    static_assert(!query<SameAs<MyLayer>, After>,
      "MyLayer must not appear twice");
    return true;
  }

  template<typename O>
  struct Part : O {
    using Base = O;

    // Optional: publish a tag so other layers can detect your presence
    using IsMyLayer = std::true_type;

    // Override methods you care about — forward everything else
    void put(char c) {
      // transform or filter
      Base::put(c);  // always forward unless intentionally suppressing
    }

    // Add new methods — they become part of the composed API
    void myMethod() { /* ... */ }
  };
};
```

**Rules of thumb:**
- Always forward to `Base::method()` unless intentionally suppressing output
- Publish `using IsXxx = std::true_type` if other layers need to detect you via `query`
- Keep `Part` stateless where possible — every data member costs RAM on every instance
- Use `SizeT` instead of `size_t` for AVR compatibility
- Write `rules<Before, After>()` for any ordering constraint — zero runtime cost

---

## Writing a Predicate

A predicate is any type exposing `template<typename O> struct Check { static constexpr bool value; }`.

```cpp
// Example: match any type that publishes `using IsMyLayer = std::true_type`
struct IsMyLayer {
  template<typename O>
  struct Check {
    static constexpr bool value = std::is_same_v<typename O::IsMyLayer, std::true_type>;
  };
};

// Usage
static_assert(query<IsMyLayer, Before>, "MyLayer must come before this");
static_assert(query<IsMyLayer, Chain<A, B, C>>, "chain must contain MyLayer");
```

---

## Writing Rules

Rules are declared as a static `constexpr bool rules()` method on the layer struct. `HasRules` detects the method automatically — no registration needed.

```cpp
struct B {
  template<typename Before, typename After>
  static constexpr bool rules() {
    // Presence: A must appear somewhere before B
    static_assert(query<SameAs<A>, Before>, "B requires A before it");

    // Uniqueness: B must not appear twice
    static_assert(!query<SameAs<B>, After>, "B must not appear twice");

    // Ordering: A must not appear after B
    static_assert(!query<SameAs<A>, After>, "A must be placed before B");

    return true;  // required
  }

  template<typename O>
  struct Part : O { /* ... */ };
};
```

**Available context inside `rules<Before, After>()`:**

| Expression | Meaning |
|---|---|
| `query<SameAs<X>, Before>` | `X` appears somewhere before this layer |
| `query<SameAs<X>, After>` | `X` appears somewhere after this layer |
| `!query<SameAs<X>, Before>` | `X` does not appear before this layer |
| `!query<SameAs<X>, After>` | `X` does not appear after this layer |
| `query<MyPredicate, Before>` | any type in `Before` satisfies `MyPredicate` |

Rule failures produce named `static_assert` errors at the `APIOf` instantiation site — zero runtime cost.

---

*Part of the [InternetOfPins](https://github.com/InternetOfPins) project family.*  
*Author: Rui Azevedo (neu-rah) · Azores, Portugal*
