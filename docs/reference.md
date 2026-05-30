# HAPI API Reference

> Header: `include/hapi/hapi.h`

## Namespace

```cpp
namespace hapi
```

---

# Types

## `Sz`

```cpp
using Sz = size_t;
```

Platform-sized unsigned integer type.

On AVR platforms:

```cpp
using Sz = unsigned int;
```

---

## `Nil`

```cpp
struct Nil {};
```

Empty sentinel type.

---

# Variables

## `query<Q,O>`

```cpp
template<typename Q,typename O>
constexpr const bool query;
```

Compile-time `Q` predicate checker over object `O`.

Evaluates:

```cpp
Q::template Check<O>::value
```

### Example

```cpp
query<SameAs<int>,int>
```

this variable will get specialized for other data structures providing a clean traversal with user predicates.

---

# Templates

## `APIOf`

### Declaration

```cpp
template<typename API, typename... OO>
struct APIOf;
```

Builds an API by composing `OO...` over `API`.

### Specialization

```cpp
template<typename API>
struct APIOf<API>;
```

Pass-through specialization when no layers are provided.

---


## `Chain`

### Declaration

```cpp
template<typename...>
struct Chain;
```

Compile-time type list and composition utility.

---

## `Chain<>`

### Members

#### `Types`

```cpp
using Types = Chain<>;
```

#### `size`

```cpp
static constexpr const Sz size{0};
```

#### `App`

```cpp
template<typename... XX>
using App = Chain<XX...>;
```

Prepends types.

#### `Ins`

```cpp
template<typename... XX>
using Ins = Chain<XX...>;
```

Appends types.

#### `Map`

```cpp
template<template<typename> class M>
using Map = Chain<>;
```

Maps a metafunction over the chain.

#### `Part`

```cpp
template<typename T>
struct Part : T;
```

Identity composition layer.

---

## `Chain<O,OO...>`

### Members

#### `Types`

```cpp
using Types = Chain<O,OO...>;
```

#### `Head`

```cpp
using Head = O;
```

#### `Tail`

```cpp
using Tail = Chain<OO...>;
```

#### `size`

```cpp
static constexpr const Sz size{1+sizeof...(OO)};
```

#### `App`

```cpp
template<typename... XX>
using App = Chain<XX...,O,OO...>;
```

Prepends types.

#### `Ins`

```cpp
template<typename... XX>
using Ins = Chain<O,OO...,XX...>;
```

Appends types.

#### `Map`

```cpp
template<template<typename> class M>
using Map = Chain<M<O>,M<OO>...>;
```

Maps a metafunction over all types.

#### `Part`

```cpp
template<typename T>
struct Part;
```

Builds nested composition:

```cpp
O::template Part<
  Chain<OO...>::template Part<T>
>
```
### Notes:

> `query` template variable will get a specialization for `Chain` so that:

```c++
query<SameAs<int>,Chain<bool,int,char>>
```

_will traverse the chain checking the predicate over each element._

---

## `CRTP`

### Declaration

```cpp
template<typename O>
struct CRTP;
```

Optional CRTP helper base.

### Members

#### `Obj`

```cpp
using Obj = O;
```

#### `obj`

```cpp
O& obj();
const O& obj() const;
```

Returns derived object reference.

#### `operator->`

```cpp
O* operator->();
const O* operator->() const;
```

Returns derived object pointer.

---

## `HasRules`

### Declaration

```cpp
template<typename T, typename = void>
struct HasRules;
```

Detects presence of:

```cpp
T::template rules<void,void>()
```

### Base

Inherits:

```cpp
std::false_type
```

### Specialization

```cpp
template<typename T>
struct HasRules<
  T,
  std::void_t<
    decltype(T::template rules<void,void>())
  >
>;
```

Inherits:

```cpp
std::true_type
```

---

## `RuleLayer`

_this is an internal aux class_

### Declaration

```cpp
template<
  typename Current,
  typename Before,
  typename After,
  bool = HasRules<Current>::value
>
struct RuleLayer;
```

Conditional rule aggregation layer.

---

## `RuleLayer<...,false>`

### Members

#### `Part`

```cpp
template<typename O>
struct Part : O;
```

Imports:

```cpp
using O::rules;
```

---

## `RuleLayer<...,true>`

### Members

#### `Part`

```cpp
template<typename O>
struct Part : O;
```

#### `rules`

```cpp
static constexpr bool rules();
```

Evaluates:

```cpp
Current::template rules<Before,After>() && O::rules()
```

---

## `SameAs`

### Declaration

```cpp
template<typename Q>
struct SameAs;
```

Equality predicate.

### Nested Templates

#### `Check`

```cpp
template<typename O>
struct Check;
```

#### `value`

```cpp
static constexpr const bool value;
```

Evaluates:

```cpp
std::is_same_v<O,Q>
```

---

# Specializations

## `query<Q,Chain<OO...>>`

```cpp
template<typename Q,typename... OO>
constexpr const bool query<Q,Chain<OO...>>;
```

Evaluates query across all chain members:

```cpp
(query<Q,OO> || ...)
```

Returns `true` if any type matches.
