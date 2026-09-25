# Open Derivation: translator prototype

This directory prototypes a proposed C++ extension, **late derivation**. `A:B` is the class `A` re-declared with base `B`,
so a class's base is chosen where the class is used. `translate.py` lowers the new syntax, source to source, into the HAPI
`Part<O>` form that avr-gcc 7.3 already accepts. For the results, see [FINDINGS.md](FINDINGS.md).

```c++
struct Twice { static int f(int x) {return 2*super::f(x);} };   // open: names `super`
struct Id    { static int f(int x) {return x;} };                // closed: can only be the last operand
using My = Twice:Id;                                             // My::f(21) == 42

template<u8 k, typename... OO>
using Cell = (OO : ... : Bias<k> : API);                         // right fold over ':'
```

## Usage

```sh
python3 translate.py IN [-o OUT]            # one file (stdout without -o)
python3 translate.py --outdir DIR IN...     # several files, same basenames under DIR
python3 translate.py --report ...           # one line per lowering on stderr
python3 translate.py --lower=nested ...     # A::Part<B::Part<C>> instead of hapi::Chain<A,B>::Part<C> (see below)
python3 tokdiff.py A B                      # are two sources equal modulo whitespace? (tokens + comments)
```

Errors are reported compiler-style, `file:line:col: error: [od-ruleN] ...`, and set exit status 1.
The `[od-ruleN]` tag names the rule of the proposal that the input breaks. `[od-scope]` marks input that is outside this prototype's scope.

## What it lowers

| input | `--lower=chain` (default: HAPI's form) | `--lower=nested` |
|---|---|---|
| `struct P { ...super::f()... };` | `struct P {template<typename O> struct Part:O { using Base=O; using Base::Base; ...Base::f()... };};` | same |
| `template<u8 k> struct Bias {...};` | same, with the template head kept on the outer holder | same |
| `using X = A:B:C;` | `using X = hapi::Chain<A,B>::Part<C>;` | `using X = A::Part<B::Part<C>>;` |
| `A:(B:C)` | flattened: `hapi::Chain<A,B>::Part<C>` | `A::Part<B::Part<C>>` |
| `(OO : ... : T)` | `hapi::Chain<OO...>::Part<T>` | `od::FoldT<T,OO...>` ([support/od_fold.h](support/od_fold.h)) |
| `(OO : ... : P : T)` | `hapi::Chain<OO...,P>::Part<T>` | `od::FoldT<T,OO...,P>` |
| `struct Z : A:B {};` | `struct Z : hapi::Chain<A>::Part<B> {};` | `struct Z : A::Part<B> {};` |

Inside an open class body:
- An unqualified `super` becomes `Base`.
- `using super::super;` is dropped, because the lowering always adds `using Base::Base;` (rule 6).
- The class's own name, when unqualified and without template arguments, becomes `Part`. This covers constructors, `A&` and `A::x`, and implements rule 1's rebinding.

In a template, `typename`/`::template` are added when an operand names a template parameter that is in scope.

**Scope and restrictions.**
- **What makes a class open:** a class is *open* (lowered to a `Part` holder) when its body names `super` unqualified.
  - Ordinary lookup wins: a member `typedef X super;` / `using super = X;`, or a namespace-scope declaration of `super`, leaves the class alone.
  - Every other class is *closed* and passes through untouched.
  - A closed class used as a left operand is refused (rule 7).
- **Refused input:**
  - An open class that already has a base, or one defined out of line (rule 8, `[od-scope]`).
  - An anonymous class, or a union, as a left operand.
  - A left fold, or an unparenthesized fold.
- **Only the prototype's grammar is recognized.** `:` is rewritten only in an alias-declaration RHS and in a base clause (rule 4). Everything else, including `super` inside `#define`s, is left untouched.
- **You must provide the includes:**
  - The input must include `<hapi/chain.h>` (or `hapi.h`) itself.
  - For `--lower=nested` with folds, it must also include `od_fold.h`.

## Layout and how to run

Requirements: `g++`, `clang++`, `python3`. Round 2 also needs `avr-g++` 7.3, `avr-objdump`/`avr-size` and `simavr`. On Ubuntu:
`apt-get install gcc-avr binutils-avr avr-libc simavr`, which gives avr-gcc 7.3.0 and simavr 1.6. Each round has a `run.sh`
that does everything and writes its logs next to it (`log.txt`, and `log/` for round 2). `HAPI=<include dir>` overrides the repo's `include/`.

| dir | what | run |
|---|---|---|
| `round1/` | one alias, one open class, one closed terminal; bare use must not compile | `round1/run.sh` |
| `round2/` | static_net's `waveCell.h` / `linCell.h` in `:` syntax (`src/`), translated (`out/include/`), round trip, then `check/build.sh` unchanged, `compare_emlearn` and `measure/` in simavr, and `avr-objdump` of 27 AVR programs, original vs translated | `round2/run.sh` (a few minutes) |
| `round2/variants/` | (a) `hapi::APIOf` written in `:` syntax; (b) `Cell` as a bare pack fold instead of `APIOf`; (c) the same with `--lower=nested` | `round2/variants/run.sh` |
| `round3/` | coverage: chains in base clauses, constructors through a chain, rule-1 rebinding, family/statics, identity (incl. across TUs), `super` precedence, dependence, the label hazard; 9 translator diagnostics; 4 compiler rejections; positive cases in both lowering modes | `round3/run.sh` |

Round 2 runs `build.sh` "unchanged" by building two throwaway mirrors of `examples/static_net`: `check/`, `compare_emlearn/`
and `measure/` are copied, `models/` is linked, and `include/` is copied. In one mirror `include/{waveCell,linCell}.h` are replaced by the translated headers.
This is needed because `build.sh` hard-codes `-I../include`. Nothing under `include/hapi/` or `examples/static_net/` is modified.
