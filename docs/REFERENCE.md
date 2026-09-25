# HAPI API Reference

Hardware Abstraction Pattern Interface. Zero-cost template metaprogramming for composable hardware layers.

## Core Concept

HAPI composes functionality into compile-time chains (`Chain<OO...>`). Each layer wraps the layer below it via `Part<O> : O`, forming a single-inheritance stack ("mono_block" topology) that the compiler flattens to direct field/register access — no vtables, no indirection.

Introspection and structural transforms are a separate, parallel system: predicates and transforms are plain types with `Apply`/`Check`/`ApplyPack` members, and `Traverse<Op,Input>` is the one recursion point that knows how to walk into a container. Which types are containers, and which walks open them, is declared per type with `Expand<O>` (see [Containers](#containers-expand)); `Chain<...>` and `APIOf<...>` are built in. `Map`, `Filter`, `Any`, … are `Traverse` plugged with a different `Op`; `FindFirst` has its own short-circuit walk that reads the same `Expand`, and so do `BuildRules`/`NoCollision`.

## Building Blocks

### Chains

| Type | Purpose |
|------|---------|
| `Chain<OO...>` | Compose multiple components in order; itself usable as a component (`Chain<OO...>::Part<T>` is defined) |
| `APIOf<API, OO...>` | Close a chain into a single class deriving from all layers + a fallback `API` base; validates `rules()` via `static_assert` |
| `T::Part<O>` | Every layer's mixin — single inheritance from `O` (the composed type of everything below) |
| `Chain<OO...>::Drop<n>` | The chain without its first `n` elements (`Chain<>` if `n` runs past the end); `Drop<n>::Head` is the element at index `n` |

### Traversal & Querying

All of these are **types**, not runtime functions — they operate on the type list, not a value.

| Symbol | Shape | Description |
|--------|-------|-------------|
| `Traverse<Op,Input>` | `::Beta` | The one container-recursion point: routes to `Op::Apply<Input>` for a leaf, or folds `Op::ApplyPack<...>` over the children of a container this `Op` opens (`Chain`, or any type with an [`Expand`](#containers-expand) entry). A hand-written `Traverse<Op,X<...>>` specialization still wins over this default |
| `Eval<Op,Input>` | alias | `= Traverse<Op,Input>::Beta` |
| `FindFirst<Q>` | `::Check<Input>` | First match of predicate `Q`, walking head/tail (own recursion, not via `Traverse`). A container whose `Expand` sets `searched` is tested as a whole first, then opened. **Hard-fails to compile** on a miss — there's no `::Result` member to name |
| `FindFirstOr<Q,Default>` | `::Check<Input>` | Same walk as `FindFirst`, but yields `Default` on a miss instead of failing |
| `Exists<Q,Input>` | alias (`bool_constant`) | Presence-only check via `Any<Q>`; never fails to compile |
| `Any<Q>` | `::Check<Input>` | Fold: `true` if `Q` matches any element (`OO::value \|\| ...` over the whole tree) |
| `query<Q,O>` | `constexpr bool` variable template | `= Exists<Q,O>::value`; the runtime-usable boolean form used by `Requires`/`Excludes` |
| `Distinct<L>` | `constexpr bool` variable template (`rules.h`) | `true` when no layer of the list `L` occurs twice, on exact types. `L` is flattened first: a nested `Chain` is spliced; a type with `::Types` (a named composition, an `APIOf`) is replaced by its `Types`, recursively; a type with `Part<O>` is an open layer (compared by `is_same`); anything else is a closed operand (compared by `is_same`, and by `is_base_of` either way with the other closed ones). `LayersOf<L>` is the flattened list. Use: `static_assert(Distinct<Chain<A,B,OO...,T>>, "...")` |
| `Expand<O>` | `::Children` + policy bits | What a container holds. Primary is declared, never defined (a leaf); specialize it per exact type. See [Containers](#containers-expand) |
| `Expansion<Kids,Queried,Selected,Validates,Searched>` | base for an `Expand` entry | Provides `Children = Kids` and the four bits, all defaulting to `false` |
| `IsContainer<O>` | `bool_constant` | `true` if `O` has an `Expand` entry |

### Functional Transforms

| Symbol | Shape | Description |
|--------|-------|-------------|
| `Map<F>` | `::Check<Input>` via `Traverse`, or `Eval<Map<F>,Input>` | Leaf-level transform: each `O` becomes `F<O>::Type`. `F` is a template-template parameter, not an object — it must expose `::Type`. Recurses structurally into nested `Chain`s |
| `Transform<F,Input>` | alias | `= Eval<Map<F>, Input>` — the usual way to invoke `Map` |
| `Filter<Q>` | `Eval<Filter<Q>,Input>` | Keeps only elements matching predicate `Q`, splicing nested-`Chain` results back into one flat result via `ConcatChains` |
| `Partition<Q,L=Left,R=Right>` | `Eval<Partition<Q>,Input>` | Wraps every element: `Q`-matches become `L<O>` (default `Left<O>`), non-matches become `R<O>` (default `Right<O>`) |
| `At<idx,O>` | `::Type` | Walks `O`'s **assembled-object** inheritance chain via `::Base` (not a `Chain<>` type list) `idx` levels up. No default/fallback — over-indexing is a hard compile error |
| `at<idx,ref>()` | function | Runtime accessor: `static_cast`s `ref` to `At<idx,decltype(ref)>::Type&` |
| `find<Q>(C& c)` | function | Compile-time gate + pass-through: `static_assert`s `Q` exists in `C::Types`, then returns `c` unchanged (by reference, const-ness preserved via deduction). It is **not** extraction — nothing is drilled into or returned besides the original object |

> **Naming overlap:** `Chain<O,OO...>` also has its own member alias `Map<template<typename> class M>` (in `chain.h`) — a *shallow*, single-level `Chain<M<O>, M<OO>...>` that applies `M<O>` directly (no `::Type` unwrap) and does **not** recurse into a nested `Chain`. It predates and is unrelated to `hapi::Map<F>` above, which is the structural, `Traverse`-based transform. Same name, different mechanism — don't reach for one expecting the other's behavior.

### Containers: `Expand`

`Expand<O>` says what a container holds, once, for every walk. An entry derives from `Expansion<Children, Queried, Selected, Validates, Searched>`; each bit says whether one family of walks opens the container (a leaf, or a bit that is off, means the element is taken whole):

| bit | opened by |
|---|---|
| `queried` | `Any`, `Exists`, `query`, `Requires`, `Excludes` (operations with `static constexpr bool is_query = true`; today only `Any`) |
| `selected` | every other `Traverse` operation: `Filter`, `Map`/`Transform`, `Partition`, … |
| `validates` | `BuildRules`, `NoCollision`: children are spliced in place and their `rules()` run with the enclosing chain as `Before`/`After`; the container's own `rules()` runs too (it sees its later siblings, not its children) |
| `searched` | `FindFirst`, after testing the container itself |

Built-in entries: `Chain<OO...>` — all four bits, children `Chain<OO...>`; `APIOf<API,OO...>` — `validates` only, children `Chain<API,OO...>` (= `APIOf::Types`). Entries are per **exact** type: a `::Types` member does not make a type a container, and a derived type needs its own entry (`template<typename... OO> struct Expand<D<OO...>> : Expand<B<OO...>> {};`). Declare an entry before the type is first queried. Leaf detection costs one class template instantiation per element visited.

```cpp
template<typename... II>
struct Expand<Box<II...>> : Expansion<Chain<II...>, /*queried*/true, /*selected*/true> {};
```

## Predicates

Predicates are plain types with three members, so `Traverse` can plug them in generically:

```cpp
template<typename Q>
struct SameAs {
  template<typename O> using Apply    = std::is_same<Q,O>;             // leaf-level test
  template<typename O> using Check    = typename Traverse<SameAs<Q>,O>::Beta; // whole-tree walk
  template<typename... OO> using ApplyPack = Chain<OO...>;             // how Traverse folds a Chain
};
```

Built-in predicates: `SameAs<Q>`, `TagIs<Tag>` (checks `std::is_base_of<Tag,O>` — the outer-struct tagging convention), `IsInstanceOf<Wrapper>`, `FromTypes<Q>` (drills into `O::Types`). Combinators: `Not<Q>`, `And<A,B>`, `Or<A,B>`.

Tags are ordinary user-defined marker structs (e.g. `struct aTag {};`) that a layer inherits from at the **outer struct** level so `TagIs<aTag>` can see it without instantiating `Part`. HAPI ships no built-in tag names — every tag is downstream-library-defined (see each library's own docs for its conventions).

## Integration with IOP Libraries

- **OnePin**: Composes port and mask layers via `APIOf`
- **OneMenu**: Chains menu printers and item definitions
- **OneOutput**: Formats and buffers output via component layers

## Rationale

HAPI provides compile-time composition that looks like inheritance but generates zero-overhead machine code. Drivers are modular and reusable while remaining as fast as hand-written assembly.

---

## See Also

- [OneChip](../../../OneChip/docs/REFERENCE.md) — Hardware platform layers
- [OneMenu](../../../OneMenu/docs/REFERENCE.md) — Menu framework
- [OneData](../../../OneData/docs/REFERENCE.md) — Data observation layer
