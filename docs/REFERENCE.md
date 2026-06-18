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
- [Indexed Storage](#indexed-storage)
- [Topology Visitors](#topology-visitors)
- [Runtime Pipeline](#runtime-pipeline)
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
| `Build<W>` | alias template | Unpack into wrapper: `W<O, OO...>` |
| `Part<T>` | struct template | Fold into mixin inheritance hierarchy over `T` |

#### `Chain<O,OO...>::Part<T>`

Folds the type list into a single class by recursive inheritance.
`Chain<A, B, C>::Part<Base>` expands to `A::Part< B::Part< C::Part<Base> > >`.
`A` is outermost — receives calls first. `Base` is innermost.

Each `Part<T>` also exposes `using Types = Chain<O, OO...>` so `query<>` can traverse it automatically.

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

### `TagIs<Tag>`

```cpp
template<typename Tag>
struct TagIs {
  template<typename O>
  struct Check : std::is_base_of<Tag, O> {};
};
```

True when `O` publicly inherits `Tag`. Use with tag structs declared on the outer component struct (not inside `Part`) so the predicate is visible to `query<>` and `rules<>` without instantiating `Part`:

```cpp
struct aFormat {};
struct MyFmt : aFormat {           // outer struct inherits the tag
  template<typename O>
  struct Part : O { ... };
};

query<TagIs<aFormat>, Chain<MyFmt, Other>>  // true — no Part instantiation needed
```

Used by `idx<I>()` internally: `find<TagIs<IdxTag<I>>>(node)`.

### `is_predicate<Q>`

```cpp
template<typename Q, typename = void>
struct is_predicate : std::false_type {};
template<typename Q>
struct is_predicate<Q, std::void_t<decltype(Q::template Check<void>::value)>>
  : std::true_type {};
```

Detects whether `Q` is a valid HAPI predicate (has a nested `Check<O>::value`). Used by `find(Q)` tag-dispatch overloads to give a readable `static_assert` when a non-predicate type is accidentally passed.

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
// Single type (primary template — F must expose Apply<T>::Expr)
template<typename F, typename O>
struct Map { using Expr = typename F::template Apply<O>::Expr; };

// Chain specialisation
template<typename F, typename... OO>
struct Map<F, Chain<OO...>> {
  using Expr = Chain<typename Map<F, OO>::Expr...>;
};

// APIOf specialisation — two dispatch paths:
//   type-level F (has Apply<T>::Expr):  maps each component descriptor
//   value-level F (constexpr callable): prepends Mapped<F> to the chain
template<typename F, typename API, typename... OO>
struct Map<F, APIOf<API, OO...>> {
  using Expr = /* APIOf<API, Map<F,OO>...>  or  APIOf<API, Mapped<F>, OO...> */;
};
```

Type-level detection via `is_type_transformer<F>`: if `F::template Apply<Nil>::Expr` exists, `F` is treated as a type transformer (existing component-mapping behaviour). Otherwise `F` is treated as a constexpr value functor and `Mapped<F>` is prepended as a chain component.

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

### `Requires<X, Chains...>`

```cpp
template<typename X, typename... Chains>
inline constexpr bool Requires = (query<X, Chains> || ...);
```

True when predicate `X` matches at least one element in any of `Chains`. Convenience wrapper over `query<>` for `rules()` expressions.

### `Excludes<X, Chains...>`

```cpp
template<typename X, typename... Chains>
inline constexpr bool Excludes = (!query<X, Chains> && ...);
```

True when predicate `X` matches no element in any of `Chains`.

```cpp
template<typename Before, typename After>
static constexpr bool rules() {
  static_assert(Requires<TagIs<aFormat>, After>,       "format layer required below");
  static_assert(Excludes<SameAs<MyLayer>, Before>,     "MyLayer must not precede this");
  static_assert(Excludes<SameAs<MyLayer>, After>,      "MyLayer must appear only once");
  return true;
}
```

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

`APIOf` exposes `using Types = Chain<API, OO...>` where `Head = API` (the terminal) and `Tail = Chain<OO...>` (the component list). `find<>` and `forEach<>` use this split to search only the component list.

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
// 1. Direct: test O against predicate Q
template<typename Q, typename O, typename = void>
constexpr bool query = Q::template Check<O>::value;

// 2. Auto-traversal: if O exposes ::Types, check O itself then search its chain
template<typename Q, typename O>  // selected when O::Types exists
constexpr bool query<Q, O, std::void_t<typename O::Types>> = []() {
  return Q::template Check<O>::value || query<Q, typename O::Types>;
}();

// 3. Chain fold: OR across all elements (recursive — handles nested sub-chains)
template<typename Q, typename... XX>
constexpr bool query<Q, Chain<XX...>> = (query<Q, XX> || ...);
```

Path 3 recurses via `query<Q, XX>` (not just `Check<XX>::value`), so nested `Chain<...>` elements inside the chain are also searched transitively.

Any composed type that exposes `Types` (which `Chain::Part` does) is automatically queryable without a manual specialisation.

**Usage:**

```cpp
query<SameAs<A>, Chain<A, B, C>>     // true
query<SameAs<A>, MyDevice>           // true if Q matches MyDevice directly, or if MyDevice::Types contains A
query<Not<SameAs<B>>, Before>        // true if B is not in Before
```

### `find<Q>(node)`

```cpp
template<typename Q, typename Node>
constexpr auto& find(Node& node) noexcept;

template<typename Q, typename Node>
constexpr auto& find(const Node& node) noexcept;
```

Locates the first component in `node`'s chain satisfying predicate `Q` and returns a reference to the collapsed `Part<...>` base. The cast is a `static_cast` — zero runtime overhead.

```cpp
hapi::find<SameAs<Id<42>>>(node)
```

**Requirements on `node`:** must be an `APIOf` node. Passing a raw `Chain::Part` fires a `static_assert`.

**Guard:** asserts `query<Q, Hapis>` at compile time so missing components produce a clear error rather than a substitution failure wall.

**Body items:** if `Q` matches a body item rather than a chain component, `find` fires a `static_assert` referencing `findBody<>`. Override `find` in the containing wrapper to fall back to a body search.

**Non-predicate guard:** fires `is_predicate` `static_assert` with a descriptive message when a non-predicate type is passed.

### `node.query<Q>()` / `node.query(Q{})`

Member query forms are **not** provided by HAPI core (the `Hapi<T>` wrapper that would supply them is currently disabled). Use the `query<Q, NodeType>` variable template directly:

```cpp
if constexpr (query<SameAs<WrapNav>, decltype(node)>) { /* ... */ }
// or with full type:
if constexpr (query<SameAs<WrapNav>, MyNodeType>) { /* ... */ }
```

---

## Indexed Storage

Heterogeneous indexed access for homogeneous-type chains. `value<K>()` counts down through the chain, giving O(1) access to element K. `operator[](i)` provides runtime index dispatch with the same mechanism.

### `At<N, T>`

```cpp
template<std::size_t N, typename T = int>
struct At {
  template<typename O>
  struct Part : O {
    T data{};

    T&       get()       noexcept;
    const T& get() const noexcept;
    void     set(const T& v) noexcept;
    operator T&()       noexcept;        // implicit conversion
    operator const T&() const noexcept;

    template<std::size_t K> constexpr auto& value();        // K==0 → data; K>0 → Base::value<K-1>()
    template<std::size_t K> constexpr const auto& value() const;
    template<typename TT = T> constexpr TT& operator[](std::size_t i);
    template<typename TT = T> constexpr const TT& operator[](std::size_t i) const;
  };
};
```

`N` is a logical index that allows reordering without changing the chain position. Three `At<0,T>`, `At<1,T>`, `At<2,T>` in a chain produce a 3-element typed array accessible via `value<K>()`.

**Constructor:** `Part` accepts a leading value argument for `data`, forwarding the rest to `Base`. This enables aggregate-style initialisation of the whole chain.

### `Mapped<F>`

```cpp
template<typename F>
struct Mapped {
  template<typename O>
  struct Part : O {
    static constexpr F fn{};   // zero storage: EBO for stateless functors

    template<std::size_t K>
    constexpr auto value() const { return fn(Base::template value<K>()); }

    constexpr auto operator[](std::size_t i) const {
      return fn(static_cast<const Base&>(*this)[i]);
    }
  };
};
```

Wraps the underlying `value<K>()` / `operator[]` walk and applies `F` to every result. Inserted automatically by `Map<F, APIOf<...>>` when `F` is a value-level functor (no `Apply<T>::Expr`).

### `IdxTag<I>` and `idx<I>(c)`

```cpp
template<std::size_t I>
struct IdxTag {
  template<typename O>
  struct Part : O { using Base::Base; };   // zero overhead (EBO)
};

// Free function: find the component tagged IdxTag<I>
template<std::size_t I, typename C>
inline decltype(auto) idx(C& c);         // → find<TagIs<IdxTag<I>>>(c)

template<std::size_t I, typename C>
inline decltype(auto) idx(const C& c);
```

Place `IdxTag<I>` immediately before the component it names, then use `idx<I>(node)` to obtain a direct reference to that component's `Part` base:

```cpp
using MyNode = APIOf<API, IdxTag<0>, Sensor, IdxTag<1>, Actuator>;
MyNode node;
idx<0>(node).read();    // → Sensor::Part<...>&
idx<1>(node).write(v);  // → Actuator::Part<...>&
```

---

## Topology Visitors

### `forEach<Q>(node, fn)`

```cpp
template<typename Q, typename Node, typename Fn>
void forEach(Node& node, Fn&& fn);

template<typename Q, typename Node, typename Fn>
void forEach(const Node& node, Fn&& fn);
```

Visits every component in `node`'s chain that satisfies predicate `Q`, calling `fn(part)` on each. `fn` receives a reference to the component's collapsed `Part<...>` base — same type that `find<Q>` returns, so all methods of that component and everything below it in the chain are accessible.

Uses `static_cast` throughout — zero runtime overhead (all casts are resolved at compile time via the mono_block topology).

```cpp
// call enable(false) on every WrapNav component
forEach<TagIs<aWrapNav>>(node, [](auto& part) { part.enable(false); });
```

**Implementation:** `forEachIn<Q, API>(Chain<...>{}, node, fn)` recursively walks the component list. Nested sub-chains are descended into. The `API` suffix is threaded through so each cast targets the exact collapsed type.

---

## Runtime Pipeline

**Header:** `include/hapi/run.h` · **Namespace:** `hapi::run`

A set of HAPI layers for composing runtime value predicates and transform pipelines without virtual dispatch. All components use the same `Chain::Part` mechanism as compile-time HAPI.

### Fold Terminals

```cpp
struct True {
  template<typename T> constexpr bool check(const T&) const { return true; }
};

struct False {
  template<typename T> constexpr bool check(const T&) const { return false; }
};
```

Use `True` as the terminal for AND-folded predicate chains, `False` for OR-folded chains.

### Runtime Predicates (AND fold)

Each component holds a runtime `q` data member and folds its result into `O::check(v)` via `&&`.

```cpp
template<typename Q> struct Equal   { template<typename O> struct Part : O { Q q{}; bool check(const Q& v) const; }; };
template<typename Q> struct Less    { template<typename O> struct Part : O { Q q{}; bool check(const Q& v) const; }; };
template<typename Q> struct Greater { template<typename O> struct Part : O { Q q{}; bool check(const Q& v) const; }; };
```

Set `q` after construction; the check returns `(v op q) && O::check(v)`.

```cpp
// range: v > 2 && v < 10
using Range = hapi::APIOf<True, Greater<int>, Less<int>>;
Range r;
hapi::find<hapi::SameAs<Greater<int>>>(r).q = 2;
hapi::find<hapi::SameAs<Less<int>>>(r).q = 10;
r.check(5);   // true
r.check(10);  // false
```

### Runtime Predicates (OR fold)

Same shape but folds via `||`. Use `False` as terminal.

```cpp
template<typename Q> struct EqualOr   { ... bool check(const Q& v) const { return (v == q) || O::check(v); } };
template<typename Q> struct LessOr    { ... };
template<typename Q> struct GreaterOr { ... };
```

### `run::Not<P>`

```cpp
template<typename P>
struct Not {
  template<typename O>
  struct Part : P::template Part<O> {
    template<typename T>
    bool check(const T& v) const { return !Base::check(v); }
  };
};
```

Wraps `P`'s Part and inverts its `check` result.

### Constexpr Transform Pipeline

```cpp
struct Identity {
  template<typename T> constexpr T transform(T&& v) const { return std::forward<T>(v); }
};

template<auto fn>   // fn is an NTTP — function pointer, constexpr lambda, or functor instance
struct Trans {
  template<typename O>
  struct Part : O {
    template<typename T>
    constexpr auto transform(T&& v) const { return fn(O::transform(std::forward<T>(v))); }
  };
};
```

Compose a chain of stateless transforms. Outermost `Trans` applies last. Terminal: `Identity`.

```cpp
using Pipeline = hapi::APIOf<Identity, Trans<double_it>, Trans<add_one>>;
Pipeline p;
p.transform(3);  // → add_one(double_it(3)) = 7
```

### Mutation Pipeline

Imperative in-place mutation: `fn(v)` is called for each component in chain order.

```cpp
struct MutBase {
  template<typename T> constexpr void run(T&) const noexcept {}
};

template<auto fn>
struct Mutate {
  template<typename O>
  struct Part : O {
    template<typename T>
    constexpr void run(T& v) const noexcept { fn(v); Base::run(v); }
  };
};
```

```cpp
using Pipe = hapi::APIOf<MutBase, Mutate<clamp>, Mutate<log_value>>;
Pipe p;
p.run(val);  // clamp(val); log_value(val);
```

### Ref / CtRef Pipeline

Each component holds a runtime pointer to an external `T`; `run()` applies `F` to `*target` for each.

```cpp
struct RefBase { constexpr void run() const noexcept {} };

template<typename T, typename F>
struct Ref {
  template<typename O>
  struct Part : O {
    T* target{};
    static constexpr F fn{};
    void run() noexcept { fn(*target); Base::run(); }
  };
};

// CtRef: array pointer baked into the type as NTTP — zero bytes in the object.
// Arr must have static storage duration.
template<std::size_t I, typename T, auto fn, T* Arr>
struct CtRef {
  template<typename O>
  struct Part : O {
    void run() noexcept { fn(Arr[I]); Base::run(); }
  };
};
```

`Ref<T,F>` stores the pointer at runtime (set after construction, e.g. via `forEach`). `CtRef<I,T,fn,Arr>` bakes the array address as a NTTP — equivalent to `DataRef<address>` in OneMenu, with no runtime indirection.

---

## Writing a Layer

```cpp
struct MyLayer {
  // Optional: validate stack ordering
  template<typename Before, typename After>
  static constexpr bool rules() {
    static_assert(Requires<TagIs<aRequiredTag>, Before>,
      "a tagged component must come before MyLayer");
    return true;
  }

  template<typename O>
  struct Part : O {
    using Base = O;
    using Base::Base;

    void myMethod() {
      // ...
      Base::myMethod();  // forward — omit only when intentionally suppressing
    }
  };
};
```

- Forward to `Base::method()` unless intentionally suppressing
- Declare tags on the outer struct (not inside `Part`) for `rules<>` / `query<>` visibility: `struct MyLayer : aTag { ... }`
- Keep `Part` stateless where possible — data members cost RAM on every instance
- Use `SizeT` instead of `size_t` for AVR compatibility

---

## Writing Rules

```cpp
struct B {
  template<typename Before, typename After>
  static constexpr bool rules() {
    static_assert(Requires<SameAs<A>, Before>,     "B requires A before it");
    static_assert(Excludes<SameAs<B>, Before>,     "B must not appear twice");
    static_assert(Excludes<SameAs<B>, After>,      "B must not appear twice");
    return true;
  }
  // ...
};
```

| Expression | Meaning |
|---|---|
| `Requires<X, Before>` | `X` matches something before this layer |
| `Requires<X, After>` | `X` matches something after this layer |
| `Excludes<X, Before>` | `X` matches nothing before this layer |
| `Excludes<X, After>` | `X` matches nothing after this layer |
| `Requires<MyPredicate, Before>` | any type in `Before` satisfies `MyPredicate` |

`Requires` and `Excludes` are thin wrappers over `query<>` — you can use `query<>` directly for multi-chain checks or other compositions.

Rule failures are `static_assert` errors reported at the `APIOf` instantiation site — zero runtime cost.

---

*Part of the [InternetOfPins](https://github.com/InternetOfPins) project family.*  
*Author: Rui Azevedo (neu-rah) · Azores, Portugal · MIT License*
