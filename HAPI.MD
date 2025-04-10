# HAPY

**The happy API**

is a generic and minimalistic composition API to support further development of modular API's

## building an API

### we can

>provide functionality with each part and have compile time errors if the functionality is not present/available in the composition.

or

>Have fallback methods that usually do nothing. This will erase all calls.  
>No compile time errors, but also no runtime code, unless an override with some functionality is provided.

also

>support CRTP style calls without infinite recursion, attempts are compile time errors.

>Provide a virtual methods cap to encapsulate different composition under the same type for generic container storage.  
>A virtual (pure or fallback) interface must be provided along with a cap to redirect calls (virtual to static definition).

>Automate creation of items parts for a specific API

## Parts

composition parts are expected to have this structure

```c++
struct MyPart {
  template<typename O> struct Part:O {
    // some API implementation 
  };
};
```

and eventually the new can be a template arbitrary parameters.

```c++
template<int defVal=30>
struct Power {
  template<typename O> struct Part:O {
    // some API implementation 
  };
};
```

## Nil

`Nil` is the default part chain terminator
```c++
struct Nil {};
```

this parts can be combined like this using c++ (tedious)

```c++
Power::Part<MyPart::Part<Nil>> example;
```

the `example` object now has api functions from MyPart::Part<Nil>
override by Power::Part. 

## How it works

```c++
template<typename... OO> struct Parts<OO...>;
```

combines a list of parts (`OO...`) into a single part.

>`Parts<A,B,C>` = `A::Part<B::Part<C>>`  
> with advantage of future expansion if `C` is a `Part`

example:
```c++
//flat composition
using Example=Parts<Power,MyPart,Nil>;
```

compiles as a class derivation  
```
Power::Part
 +MyPart::Part
  +Nil
```

defining new parts
```c++
//flat composition with no terminator
using Example=Parts<Power,MyPart>;
Parts<Example,Nil> example;//Example is a valid part
```

## Examples

### free

free use of parts, a composition of parts is also a valid part.

### flat

static and non-recursive API interface

### crtp

static, recursive and cycle safe API calls

### virt

composing items with common virtual interface

### std

store virtual interface items in std container
