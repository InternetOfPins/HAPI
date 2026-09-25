# Open Derivation: translator prototype

This directory prototypes a proposed C++ extension, **late derivation**. `A:B` is the class `A` re-declared with base `B`,
so a class's base is chosen where the class is used. `translate.py` lowers the new syntax, source to source, into HAPI:
- open classes become HAPI parts (`Part<O>`)
- a chain left open becomes a component (`hapi::Chain<...>`)
- a closed composition becomes `hapi::APIOf<Terminal,...>`, which is what starts the Part collapse

The output is plain C++17 that avr-gcc 7.3 accepts. For the results, see [FINDINGS.md](FINDINGS.md).

```c++
struct Id    { static int f(int x) {return x;} };                // closed: can only be the terminal
struct Twice { static int f(int x) {return 2*super::f(x);} };   // open: names `super`, usable as a layer
struct My : Twice:final Id {};                                   // closed on the terminal API Id: My::f(21) == 42

struct W : A:B {};                                               // no `final`: a component, open
struct X : W:final Nil {};                                       // closed where it is used, on the user's own Nil

template<u8 k, typename... OO>
struct Cell : (OO : ... : Bias<k> : final API) {};               // right fold over ':', closed on API

using Y = Twice:final Id;                                        // error [od-rule4]: ':' is only valid in a base clause
```

A closed struct is an XXXDef, in the IOP style (OneMenu's `ItemDef<OO...>`): derived from `APIOf`, with its own `hapi::Expand` entry,
which the translator emits next to it. `final T` closes on the terminal API `T`. It is always the last operand, and it becomes `APIOf`'s first parameter. A chain without `final` stays
open, so it can be used as a layer and closed later. Inside a closed struct, `super` is its base, and the struct inherits that base's constructors:
`struct A : B:final C {...}` means the same as `struct C {..}; struct B : C {..}; struct A : B {..};`.

## Usage

```sh
python3 translate.py IN [-o OUT]            # one file (stdout without -o)
python3 translate.py --outdir DIR IN...     # several files, same basenames under DIR
python3 translate.py --report ...           # one line per lowering on stderr
python3 tokdiff.py A B                      # are two sources equal modulo whitespace? (tokens + comments)
```

Errors are reported compiler-style, `file:line:col: error: [od-...] ...`, and set exit status 1.
The tag names the rule of the proposal that the input breaks.

## What it lowers

| input | output |
|---|---|
| `struct P { ...super::f()... };` (an *open* class) | `struct P {template<typename O> struct Part:O { using Base=O; using Base::Base; ...Base::f()... };};` |
| `template<u8 k> struct Bias {...};` | same, with the template head kept on the outer holder |
| `template<class Bf,class Af> static constexpr bool rules() {...}` in an open class | kept on the holder, outside `Part`, where HAPI's rule walk asks for it |
| `struct Z : A:B:final T {...};` | `struct Z : hapi::APIOf<T,A,B> {using Base=hapi::APIOf<T,A,B>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,B,T>>, "duplicate layer in Z"); ...};` |
| `super` inside `Z` above | `Base` |
| after `Z`, in `Z`'s namespace | `using Z_APIOf=hapi::APIOf<T,A,B>;` then, in `namespace hapi`, `Expand<ns::Z> : Expand<ns::Z_APIOf>` and `HasOwnRules<ns::Z> : HasOwnRules<ns::Z_APIOf>`: the XXXDef entries (template Defs get partial specializations, non-type parameters as `auto`) |
| `struct Z : final T {};` | `struct Z : hapi::APIOf<T> {...};` |
| `struct W : A:B {};` (no `final`) | `struct W : hapi::Chain<A,B> {static_assert(hapi::Distinct<hapi::Chain<A,B>>, "duplicate layer in W");};`: a component |
| `struct Z : W:C:final T {};` with `W` a component | `struct Z : hapi::APIOf<T,W,C> {...};` (`W` is spliced by HAPI's walks) |
| `struct Z : A:(B:final C) {};` | flattened, the same as `A:B:final C` |
| `template<class... OO> struct Z : (OO : ... : P : final T) {};` | `struct Z : hapi::APIOf<T,OO...,P> {...};` |
| `template<class... OO> struct W : (OO : ... : P) {};` | `struct W : hapi::Chain<OO...,P> {...};` |
| `using X = A:B;` | refused: `[od-rule4]` |

Inside an open class body:
- An unqualified `super` becomes `Base`.
- `using super::super;` is dropped, because the lowering always adds `using Base::Base;`.
- The class's own name, when unqualified and without template arguments, becomes `Part`. This covers constructors, `A&` and `A::x`.

**Checks happen in HAPI, at compile time.**
- `APIOf` runs HAPI's rule walk, i.e. the components' `rules<Before,After>()`.
- The injected `static_assert(hapi::Distinct<...>)` rejects duplicate layers on exact types: packs, aliases, `Bias<1>` vs `Bias<0+1>`, and components from other headers.
  This is the stand-in for the native error that `struct X : Nil, Nil {}` already gets.

**Scope and restrictions.**
- **What makes a class open:** a class is *open* (lowered to a `Part` holder) when its body names `super` unqualified.
  - Ordinary lookup wins: a member `typedef X super;` / `using super = X;`, or a namespace-scope declaration of `super`, leaves the class alone.
- **Refused input:**
  - a `:` in an alias-declaration
  - `final` on any operand but the last, or a closed class as the open end of a component (`[od-final]`)
  - a component with a non-empty body (`[od-component]`)
  - a closed class or a closed composition as a layer
  - rebasing: an open class with an ordinary base, or `(A:B):final C`
  - two `:` bases in one class
  - out-of-line members of an open class
  - left or unparenthesized folds
  - `super` outside a class, or inside `rules()`
- **Only the prototype's grammar is recognized.** Everything else, including `super` inside `#define`s, is left untouched.
- **You must provide the includes:** the input must include `<hapi/hapi.h>` itself.

## Layout and how to run

Requirements: `g++`, `clang++`, `python3`. Round 2 also needs `avr-g++` 7.3, `avr-objdump`/`avr-size` and `simavr`. On Ubuntu:
`apt-get install gcc-avr binutils-avr avr-libc simavr`, which gives avr-gcc 7.3.0 and simavr 1.6. Each round has a `run.sh`
that does everything and writes its logs next to it (`log.txt`, and `log/` for round 2). `HAPI=<include dir>` overrides the repo's `include/`.

| dir | what | run |
|---|---|---|
| `round1/` | one closed composition over one open class and a terminal; bare use must not compile; the alias form must be refused | `round1/run.sh` |
| `round2/` | static_net's `waveCell.h` / `linCell.h` in `:` syntax (`src/`, `Cell` closed with `final API`), translated (`out/include/`); round trip (diff = the `Cell` lines only); `check/build.sh` unchanged; `compare_emlearn` and `measure/` in simavr; `avr-objdump` of 27 AVR programs; the struct `Cell` next to `hapi::APIOf` | `round2/run.sh` (a few minutes) |
| `round2/variants/` | `hapi::APIOf` itself written in `:` syntax | `round2/variants/run.sh` |
| `round3/` | coverage: components and closing on a user terminal, base-clause chains and folds, named compositions against their plain-C++ equivalent, constructors, rule-1 rebinding, family/statics, nominal identity (incl. across TUs), `super` precedence, dependence, component `rules()`, XXXDef `Expand` entries (a Def nested in an outer rule walk), duplicate layers, the label hazard; 16 translator refusals; 16 compiler rejections | `round3/run.sh` |

Round 2 runs `build.sh` "unchanged" by building two throwaway mirrors of `examples/static_net`: `check/`, `compare_emlearn/`
and `measure/` are copied, `models/` is linked, and `include/` is copied. In one mirror `include/{waveCell,linCell}.h` are replaced by the translated headers.
This is needed because `build.sh` hard-codes `-I../include`. Nothing under `examples/static_net/` is modified.
