# HAPI Architecture & Meta-Programming Reference Manual

This manual contains the technical specifications of the HAPI compile-time transformation engine, type-level monads, and constraint validation tree.

---

## 1. Core Containers & Topology Operators

### `Chain<Ts...>`

The foundational compile-time type array. Acts as a stateless list container for physical layers or abstract metadata types.

* **State Overhead:** `0 bytes`
* **Static Accessors:**
* `size`: `static constexpr SizeT` showing total element count.
* `Head`: Alias to the first type in the sequence.
* `Tail`: Alias to a nested `Chain` containing the remaining elements (`Chain<Ts+1..._>`).


* **Type-Level Manipulators:**
* `App<Xs...>`: Prepends new types to the existing chain list: `Chain<Xs..., Ts...>`.
* `Ins<Xs...>`: Appends new types to the end of the chain list: `Chain<Ts..., Xs...>`.
* `Map<M>`: Applies an inline unary template transformation `M` to all contained types.
* `Part<T>`: Folds the type sequence right-to-left into a recursive CRTP mixin inheritance structure: `T1::Part<T2::Part<...::Part<T>>>`.



### `APIOf<API, Ts...>`

The entry-point composition boundary. Materializes a static `Chain` fold into a single operational class derived from a fallback base interface.

* **Static Asserts:** Implicitly triggers the `BuildRules` validation engine. If any constraint rule inside the tree evaluates to `false`, compilation is aborted with a validation failure.

---

## 2. Compile-Time Variadic Predicates

Predicates are evaluators used inside `query` operations to inspect composition properties during compilation.

### `SameAs<Q>`

Strict type-matching comparator.

* **Evaluation:** Evaluates to `true_type` if target type `O` matches `Q` exactly via `std::is_same_v`.

### `Not<P>`

Logical negation operator.

* **Evaluation:** Inverts the output boolean value of an existing predicate `P` against a target type.

### `And<Ps...>`

Short-circuiting variadic logical AND fold.

* **Evaluation:** Returns `true` if every predicate in `Ps...` evaluates to `true` against the target type.

### `Or<Ps...>`

Short-circuiting variadic logical OR fold.

* **Evaluation:** Returns `true` if at least one predicate in `Ps...` evaluates to `true` against the target type.

---

## 3. Monadic Channels & Transformations

HAPI introduces asymmetric type partitioning using monadic channel routing to process layers cleanly without runtime footprints.

### `Left<T>` / `Right<T>`

Monadic wrapping structures used to route elements down dual processing tracks.

* **Purpose:** Primarily used by the internal layout-filtering and feature-injection sub-systems to classify components into logical execution categories.

### `IsInstanceOf<Wrapper>`

Universal template tracking inspector.

* **Purpose:** Detects if an evaluated type is a specialization of a template class (`Left`, `Right`, `Chain`, etc.), enabling SFINAE routing without ambiguity errors.
* **Global Aliases:**
* `IsLeft`: Tracks if an object has been routed down the `Left<T>` channel.
* `IsRight`: Tracks if an object has been routed down the `Right<T>` channel.



### `Partition<Q>`

Asymmetric conditional routing transformer.

* **Mechanism:** Interrogates target type `O` via predicate `Q`. If the predicate matches, it wraps the element inside a `Right<O>`; otherwise, it falls back to a `Left<O>`.

### `Map<F, Target>`

Pure 1:1 type topology walker.

* **Application:** Evaluates a transformation structure `F` across a raw type `O` or unpacks a `Chain<OO...>` to perform element-by-element type conversions.

### `FilterIf<P, Chain>`

Conditional extraction and garbage collection mechanism.

* **Mechanism:** Traverses through compile-time lists to strip out or unpack elements based on predicate matching.
* **Specializations:**
1. **Monadic Wrappers:** Evaluates wrapped categories via `P::Check`, automatically discarding non-matching paths while unpacking valid contents into the target array via SFINAE.
2. **Recursive Sub-Chains:** Iterates downward through hardware sub-trees, extracting nested definitions into linear flat chains.



---

## 4. Multi-Directional Constraint Engine

The HAPI validation subsystem performs an automated bidirectional walk across the full stack composition before binary generation.

### Structural Interface Enforcer

To participate in the automated structural validation cycle, a custom hardware layer must expose a public static member function matching the following signature:

```cpp
template<typename Before, typename After>
static constexpr bool rules() {
  // Validate constraints using query operators:
  static_assert(query<SameAs<MySink>, After>, "MySink must sit below this layer!");
  return true;
}

```

### Engine Mechanics

```
   BuildRules< Before, After >
      │
      ▼
   Extract: Current Layer = After::Head
      │
      ├──► Check: HasRules<Current>? 
      │       ├──► Yes: Execute Current::rules<Before, After>()
      │       └──► No:  Skip to next layer
      ▼
   Recurse: Before = Before + Current
            After  = After::Tail

```

* **`HasRules<T>`:** Detects the presence of the validation signature using SFINAE (`std::void_t`). If missing, validation passes through safely.
* **`RuleLayer<Current, Before, After, Match>`:** Executes the current constraint checks and chains the evaluations recursively via the `&&` short-circuit operator down the rest of the type tree.
* **`BuildRules<Before, After>`:** Manages the active state machine of the walk. It tracks what came before, isolates the target layer, and defines what remains below, establishing absolute context for every check.

---

## 5. Global Introspection Utility

### `query<Q, O>`

The universal entry point for system introspection. Resolves types instantly down three branches:

```cpp
// Branch 1: Direct Object Evaluation
template<typename Q, typename O>
constexpr bool result = query<Q, O>;

// Branch 2: Flat Chain Expansion (Evaluates OR condition across all elements)
template<typename Q, typename... XX>
constexpr bool result = query<Q, Chain<XX...>>;

// Branch 3: Collapsed Type Object Discovery
template<typename Q, typename OperationalDevice>
constexpr bool result = query<Q, OperationalDevice>;

```

* **Device Unpacking:** If passed an active operational type containing an inner `Types` token, `query` automatically unpacks the runtime device into its original compile-time type list to validate properties instantly.