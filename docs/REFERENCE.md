# HAPI Reference Manual

> **File:** `include/hapi/hapi.h` · **Namespace:** `hapi` · **Requires:** C++17

---

## Contents

- [Core Types](#core-types)
- [Chain](#chain)
- [Predicates](#predicates)
- [Logical Combinators](#logical-combinators)
- [Monadic Channels](#monadic-channels)
- [Chain Transformations](#chain-transformations)
- [Rules System](#rules-system)
- [Composition](#composition)
- [Introspection](#introspection)
- [Writing a Layer](#writing-a-layer)
- [Writing Rules](#writing-rules)

---

## Core Types

### `Nil`

```cpp
struct Nil {};
```

Sentinel empty type. Use as a neutral base or default template argument.

---

### `SizeT`

```cpp
// AVR:    using SizeT = unsigned int;
// Other:  using SizeT = size_t;
```

Cross-platform size type. Use instead of `size_t` in layer code for AVR compatibility.

---

## Chain

### `Chain<O, OO...>`

```cpp
template<typename O, typename... OO>
struct Chain<O, OO...>;
```

Compile-time ordered type list. The foundation of open chain derivation.

| Member | Kind | Description |
|---|---|---|
| `Types` | type alias | `Chain<O, OO...>` — the list itself |
| `Head` | type alias | First type: `O` |
| `Tail` | type alias | Remaining types: `Chain<OO...>` |
| `size` | `constexpr SizeT` | Number of types |
| `App<XX...>` | alias template | Prepend: `Chain<XX..., O, OO...>` |
| `Ins<XX...>` | alias template | Append: `Chain<O, OO..., XX...>` |
| `Map<M>` | alias template | Apply `M<>` to each type: `Chain<M<O>, M<OO>...>` |
| `Part<T>` | struct template | Fold into mixin inheritance hierarchy over `T` |

#### `Chain<O,OO...>::Part<T>`

Folds the type list into a single class by recursive inheritance.
`Chain<A, B, C>::Part<Base>` expands to `A::Part< B::Part< C::Part<Base> > >`.
`A` is outermost — receives calls first. `Base` is innermost.

---

## Predicates

A predicate is any type exposing `template<typename O> struct Check { static constexpr bool value; }`.

### `SameAs<Q>`

```cpp
template<typename Q> struct SameAs {
  template<typename O> struct Check {
    static constexpr const bool value{std::is_same_v<O, Q>};
  };
};
```

True when target type `O` is exactly `Q`.

---

## Logical Combinators

Compose predicates without writing new structs.

### `Not<P>`

```cpp
template<typename P> struct Not {
  template<typename O> struct Check {
    static constexpr const bool value{!P::template Check<O>::value};
  };
};
```

True when `P` is false for `O`.

### `And<Ps...>`

```cpp
template<typename... Ps> struct And {
  template<typename O> struct Check {
    static constexpr const bool value{(Ps::template Check<O>::value && ... && true)};
  };
};
```

True when all predicates in `Ps...` are true for `O`. Empty pack → true.

### `Or<Ps...>`

```cpp
template<typename... Ps> struct Or {
  template<typename O> struct Check {
    static constexpr const bool value{(Ps::template Check<O>::value || ...)};
  };
};
```

True when at least one predicate in `Ps...` is true for `O`.

**Usage:**

```cpp
query<Not<SameAs<A>>, Chain<B, C>>              // true
query<And<SameAs<A>, Not<SameAs<B>>>, T>        // true if T==A and T!=B
query<Or<SameAs<A>, SameAs<B>>, Chain<A, C>>    // true
```

---

## Monadic Channels

Tagged wrapper types used to categorise elements during chain transformation.

### `Left<T>` / `Right<T>`

```cpp
template<typename T> struct Left  { using Type = T; ... };
template<typename T> struct Right { using Type = T; ... };
```

Wrap a type to mark it as unmatched (`Left`) or matched (`Right`) during a `Partition` or `FilterIf` pass. Each carries a `Check` that matches both the wrapper and the inner type.

### `IsInstanceOf<Wrapper>`

```cpp
template<template<typename...> class Wrapper>
struct IsInstanceOf;
```

True when the target type is a specialisation of `Wrapper`.

```cpp
IsInstanceOf<Left>::Check<Left<int>>::value   // true
IsInstanceOf<Left>::Check<Right<int>>::value  // false
```

**Convenience aliases:**

```cpp
struct IsLeft  : IsInstanceOf<Left>  {};
struct IsRight : IsInstanceOf<Right> {};
```

---

## Chain Transformations

### `Partition<Q>`

```cpp
template<typename Q> struct Partition {
  template<typename O> struct Apply {
    static constexpr bool value = Q::template Check<O>::value;
    using Expr = std::conditional_t<value, Right<O>, Left<O>>;
  };
};
```

Maps each type to `Right<T>` if it satisfies `Q`, or `Left<T>` if it does not. Used as the transformation argument to `Map`.

### `Map<F, Target>`

```cpp
// Single type
template<typename F, typename O>
struct Map { using Expr = typename F::template Apply<O>::Expr; };

// Chain specialisation
template<typename F, typename... OO>
struct Map<F, Chain<OO...>> {
  using Expr = Chain<typename Map<F, OO>::Expr...>;
};

// APIOf specialisation
template<typename F, typename API, typename... OO>
struct Map<F, APIOf<API, OO...>> {
  using Expr = APIOf<API, typename Map<F, OO>::Expr...>;
};
```

Applies transformation `F` to each type in a chain or across an `APIOf` boundary, producing a new chain of the same shape. `F` must expose `template<typename O> struct Apply { using Expr = ...; }`.

### `FilterIf<P, Chain<...>>`

```cpp
template<typename P, typename C, typename Enable = void>
struct FilterIf;
```

Traverses a chain and extracts types that satisfy predicate `P`, unwrapping their inner type in the process.

Three cases handled:

| Case | Input | Output |
|---|---|---|
| Empty chain | `Chain<>` | `Chain<>` |
| Monadic wrapper `Wrapper<T>` | matches `P` → extract `T`; no match → skip | accumulated `Chain` |
| Nested `Chain<OO...>` | recurse into sub-chain | flattened into result |

SFINAE on the monadic wrapper case prevents ambiguity with the nested chain case.

---

## Rules System

The rules system validates layer ordering at compile time. Detection and execution are fully automatic.

### `HasRules<T>`

```cpp
template<typename T, typename = void>
struct HasRules : std::false_type {};

template<typename T>
struct HasRules<T, std::void_t<decltype(T::template rules<void,void>())>>
  : std::true_type {};
```

Detects whether `T` has a `rules<Before, After>()` static method via SFINAE. No tag or marker needed on the layer.

### `RuleLayer<Current, Before, After, true>`

```cpp
template<typename Current, typename Before, typename After>
struct RuleLayer<Current, Before, After, true> {
  template<typename O> struct Part : O {
    static constexpr bool rules() {
      return Current::template rules<Before, After>() && O::rules();
    }
  };
};
```

Composes `Current`'s rules with the chain below. Only instantiated when `HasRules<Current>` is true. The false specialisation passes `O::rules` through unchanged.

### `BuildRules<Before, After>`

Walks the chain threading `Before` forward at each step:

```
BuildRules< Chain<>, Chain<A, B, C> >
  step 1: Current=A, Before=Chain<>,    After=Chain<B,C>
  step 2: Current=B, Before=Chain<A>,   After=Chain<C>
  step 3: Current=C, Before=Chain<B,A>, After=Chain<>
```

`Before` is built via `App<Head>` which prepends — so elements accumulate in reverse insertion order (`Chain<B,A>` not `Chain<A,B>`). This has no effect on rule correctness: `query` ORs over the chain and is order-agnostic.

Triggered automatically by `APIOf` — never instantiate directly.

---

## Composition

### `APIOf<API, OO...>`

```cpp
template<typename API, typename... OO>
struct APIOf : Chain<OO...>::template Part<API>;
```

Closes the chain. Collapses `OO...` into a single class deriving from `API`. Triggers `BuildRules` validation at instantiation. If any `rules<>()` fires a `static_assert`, the error is reported here.

| Parameter | Description |
|---|---|
| `API` | Innermost base — provides the public interface surface |
| `OO...` | Feature layers, outermost-first |

### `CRTP<O>`

```cpp
template<typename O> struct CRTP {
  using Obj = O;
  O&       obj();
  const O& obj() const;
  O*       operator->();
  const O* operator->() const;
};
```

Optional. Provides self-reference from any layer back to the fully-composed object via `obj()`. Use only when a layer needs to call methods on the outermost type.

> **Note:** Increases error message size significantly. Can cause infinite loops if `obj()` re-enters the same method. Use sparingly.

---

## Introspection

### `query<Q, O>`

Universal compile-time predicate query. Three resolution paths:

```cpp
// 1. Direct: test O against predicate Q (primary template, third param enables SFINAE below)
template<typename Q, typename O, typename = void>
constexpr bool query = Q::template Check<O>::value;

// 2. Auto-traversal: if O exposes ::Types, check O itself then search its chain
template<typename Q, typename O>  // selected when O::Types exists
constexpr bool query<Q, O, std::void_t<typename O::Types>> = []() {
  return Q::template Check<O>::value || query<Q, typename O::Types>;
}();

// 3. Chain fold: OR across all types in the chain
template<typename Q, typename... XX>
constexpr bool query<Q, Chain<XX...>> = (Q::template Check<XX>::value || ...);
```

Any composed type that exposes `Types` (which `Chain::Part` does) is automatically queryable without a manual specialisation. The auto-traversal specialisation (2) checks `O` itself first, then recurses into its layer list.

**Usage:**

```cpp
query<SameAs<A>, Chain<A, B, C>>     // true
query<SameAs<A>, MyDevice>           // true if Q matches MyDevice directly, or if MyDevice::Types contains A
query<Not<SameAs<B>>, Before>        // true if B is not in Before
```

### `find<Q>(node)`

```cpp
template<typename Q, typename CurrentNode>
constexpr auto& find(CurrentNode& node) noexcept;
```

Walks the inheritance chain of `node` and returns a reference to the innermost layer for which `query<Q, Base>` is false — i.e. the layer that owns the match. Useful for accessing a specific layer by tag from outside the composition.

```cpp
// tag a layer
template<int id> struct Id { template<typename O> using Part = O; };

// retrieve by tag
auto& item = mainMenu.template find<Id<42>>();
item.enable(false);
```

`Q` is typically `SameAs<Id<N>>` or any predicate matching the target layer. The search stops at the outermost layer whose `Base` no longer contains a match.

---

## Writing a Layer

```cpp
struct MyLayer {
  // Optional: validate stack ordering
  template<typename Before, typename After>
  static constexpr bool rules() {
    static_assert(query<SameAs<RequiredLayer>, Before>,
      "RequiredLayer must come before MyLayer");
    return true;
  }

  template<typename O>
  struct Part : O {
    using Base = O;
    using IsMyLayer = std::true_type;  // optional tag for query detection

    void myMethod() {
      // ...
      Base::myMethod();  // forward — omit only when intentionally suppressing
    }
  };
};
```

- Forward to `Base::method()` unless intentionally suppressing
- Publish `using IsXxx = std::true_type` for `query`-based detection
- Keep `Part` stateless where possible — data members cost RAM on every instance
- Use `SizeT` instead of `size_t` for AVR compatibility

---

## Writing Rules

```cpp
struct B {
  template<typename Before, typename After>
  static constexpr bool rules() {
    static_assert(query<SameAs<A>, Before>,   "B requires A before it");
    static_assert(!query<SameAs<B>, After>,   "B must not appear twice");
    static_assert(!query<SameAs<A>, After>,   "A must be placed before B");
    return true;
  }
  // ...
};
```

| Expression | Meaning |
|---|---|
| `query<SameAs<X>, Before>` | `X` is somewhere before this layer |
| `query<SameAs<X>, After>` | `X` is somewhere after this layer |
| `!query<SameAs<X>, Before>` | `X` is not before this layer |
| `!query<SameAs<X>, After>` | `X` is not after this layer |
| `query<MyPredicate, Before>` | any type in `Before` satisfies `MyPredicate` |

Rule failures are `static_assert` errors reported at the `APIOf` instantiation site — zero runtime cost.

---

*Part of the [InternetOfPins](https://github.com/InternetOfPins) project family.*  
*Author: Rui Azevedo (neu-rah) · Azores, Portugal · MIT License*