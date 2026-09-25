# Open Derivation: translator prototype

This directory prototypes a proposed C++ extension, **late derivation**. `A:B` is the class `A` re-declared with base `B`,
so a class's base is chosen where the class is used. `translate.py` lowers the new syntax, source to source, into the HAPI
`Part<O>` form that avr-gcc 7.3 already accepts. For the results, see [FINDINGS.md](FINDINGS.md).

**Struct-only form.** `:` derivation is valid only in a base clause, so every composition is a named struct:

```c++
struct Twice { static int f(int x) {return 2*super::f(x);} };   // open: names `super`
struct Id    { static int f(int x) {return x;} };                // closed: can only be the last operand
struct My : Twice:Id {};                                         // My::f(21) == 42

template<u8 k, typename... OO>
struct Cell : (OO : ... : Bias<k> : API) {};                     // right fold over ':'

using X = Twice:Id;                                              // error [od-rule4]: ':' is only valid in a base clause
```

`struct A : B:C {...}` means the same as `struct C {..}; struct B : C {..}; struct A : B {..};`. Inside `A`, `super` is its
base `B:C`, and `A` inherits that base's constructors.

## Usage

```sh
python3 translate.py IN [-o OUT]            # one file (stdout without -o)
python3 translate.py --outdir DIR IN...     # several files, same basenames under DIR
python3 translate.py --report ...           # one line per lowering on stderr
python3 translate.py --lower=nested ...     # EXPERIMENTAL: A::Part<B::Part<C>> / od::FoldT, no Chain wrapper (see FINDINGS)
python3 tokdiff.py A B                      # are two sources equal modulo whitespace? (tokens + comments)
```

Errors are reported compiler-style, `file:line:col: error: [od-ruleN] ...`, and set exit status 1.
The `[od-ruleN]` tag names the rule of the proposal that the input breaks. `[od-scope]` marks input that is outside this prototype's scope.

## What it lowers

| input | output (default `chain` lowering: HAPI's form) |
|---|---|
| `struct P { ...super::f()... };` (an *open* class) | `struct P {template<typename O> struct Part:O { using Base=O; using Base::Base; ...Base::f()... };};` |
| `template<u8 k> struct Bias {...};` | same, with the template head kept on the outer holder |
| `struct Z : A:B:C {...};` | `struct Z : hapi::Chain<A,B>::Part<C> {using Base=hapi::Chain<A,B>::Part<C>; using Base::Base; ...};` |
| `struct Z : A:(B:C) {};` | flattened, the same as `A:B:C` |
| `template<class... OO> struct Z : (OO : ... : P : T) {};` | `struct Z : hapi::Chain<OO...,P>::template Part<T> {using Base=typename hapi::Chain<OO...,P>::template Part<T>; using Base::Base;};` |
| `super` inside `Z` above | `Base` |
| `using X = A:B;` | refused: `[od-rule4]` |

Inside an open class body:
- An unqualified `super` becomes `Base`.
- `using super::super;` is dropped, because the lowering always adds `using Base::Base;`.
- The class's own name, when unqualified and without template arguments, becomes `Part`. This covers constructors, `A&` and `A::x`.

In a template, `typename`/`::template` are added when an operand names a template parameter that is in scope.

**Scope and restrictions.**
- **What makes a class open:** a class is *open* (lowered to a `Part` holder) when its body names `super` unqualified.
  - Ordinary lookup wins: a member `typedef X super;` / `using super = X;`, or a namespace-scope declaration of `super`, leaves the class alone.
- **Named compositions:** a class whose base clause uses `:` is a named composition. It is concrete, not open, and gets the injected `Base`.
  - Only one `:` base-specifier is allowed per class. Other, ordinary bases may sit next to it.
- **Refused input:**
  - a `:` in an alias-declaration
  - a closed class as a left operand
  - rebasing: an open class with an ordinary base, a class with a base as a left operand, or `(A:B):C`
  - two `:` bases in one class
  - out-of-line members of an open class
  - left or unparenthesized folds
  - `super` outside a class
- **Only the prototype's grammar is recognized.** Everything else, including `super` inside `#define`s, is left untouched.
- **You must provide the includes:**
  - The input must include `<hapi/chain.h>` (or `hapi.h`) itself.
  - For `--lower=nested` with folds, it must also include `support/od_fold.h`.

## Layout and how to run

Requirements: `g++`, `clang++`, `python3`. Round 2 also needs `avr-g++` 7.3, `avr-objdump`/`avr-size` and `simavr`. On Ubuntu:
`apt-get install gcc-avr binutils-avr avr-libc simavr`, which gives avr-gcc 7.3.0 and simavr 1.6. Each round has a `run.sh`
that does everything and writes its logs next to it (`log.txt`, and `log/` for round 2). `HAPI=<include dir>` overrides the repo's `include/`.

| dir | what | run |
|---|---|---|
| `round1/` | one named composition over one open class and a closed terminal; bare use must not compile; the alias form must be refused | `round1/run.sh` |
| `round2/` | static_net's `waveCell.h` / `linCell.h` in `:` syntax (`src/`, `Cell` as a struct over the fold), translated (`out/include/`); round trip (diff = the `Cell` lines only); `check/build.sh` unchanged; `compare_emlearn` and `measure/` in simavr; `avr-objdump` of 27 AVR programs, raw and with symbols stripped; the struct `Cell` next to `hapi::APIOf` | `round2/run.sh` (a few minutes) |
| `round2/variants/` | (a) `hapi::APIOf` written in `:` syntax; (b) EXPERIMENTAL `--lower=nested` through `check/build.sh` | `round2/variants/run.sh` |
| `round3/` | coverage: base-clause chains and folds, named compositions against their plain-C++ equivalent, constructors, rule-1 rebinding, family/statics, nominal identity (incl. across TUs), `super` precedence, dependence, the label hazard; 11 translator refusals; 4 compiler rejections; positive cases also under `--lower=nested` (experimental) | `round3/run.sh` |

Round 2 runs `build.sh` "unchanged" by building two throwaway mirrors of `examples/static_net`: `check/`, `compare_emlearn/`
and `measure/` are copied, `models/` is linked, and `include/` is copied. In one mirror `include/{waveCell,linCell}.h` are replaced by the translated headers.
This is needed because `build.sh` hard-codes `-I../include`. Nothing under `include/hapi/` or `examples/static_net/` is modified.
