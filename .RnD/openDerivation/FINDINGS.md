# Open Derivation: findings from the translator prototype

Base commit: `5c4bfeb`. Toolchains: g++ and clang++ (host, `-std=c++17`), avr-gcc 7.3.0, avr-binutils, simavr 1.6
(ATmega328p @ 16 MHz), all from Ubuntu 24.04 packages.

**Evidence levels:**
- **built:** compiles, or is rejected with the stated diagnostic.
- **host:** runs natively and the checks hold.
- **simulated:** runs in simavr.
- **silicon:** runs on a real Nano. None in this round: simavr has matched silicon at 0 cycles difference on this example (`examples/static_net/measure/silicon_results.md`).

Logs are in each round's `log.txt` (round 2 also has `log/`).

## 0. The form of the proposal these results are for: struct-only

- `:` derivation is valid **only in a base clause**: `struct Z : A:B:C {...};`, and pack folds `struct Z : (OO : ... : T) {};`.
- The alias form `using X = A:B;` is no longer valid, and the translator refuses it with `[od-rule4]`.
- Every struct with a `:` base clause implicitly inherits that base's constructors. The translator injects `using Base=<chain>; using Base::Base;`, as `hapi::APIOf` does by hand.
- Inside such a struct, `super` names that base: `struct A : B:C {..}` ⇔ `struct C {..}; struct B : C {..}; struct A : B {..};` *(host, `round3/src/pos/named_chain.cpp`)*.
- Base-clause chains lower to the wrapping form `hapi::Chain<...>::Part<T>`.
- **Amendment: no explicit termination.** The rightmost operand may be open (it names `super`). The chain then ends in an implicit empty
  terminal: `struct Z : A:B {};` with `B` open lowers to `hapi::Chain<A,B>::Part<od::Nil>` (`support/od_nil.h`). There is no
  "the last operand must be closed" check.
- **Amendment: no duplicate layers.** `A:X` is rejected when `A` already occurs in `X` or in `X`'s bases, by exact type match
  (`Bias<1>:Bias<2>` stays legal). The diagnostic is `[od-dup]`, at the composition site, and names both types.
- `--lower=nested` is kept as experimental only (§4).

This replaces an earlier round of the same prototype that also accepted the alias form. Its two identity findings are moot now; they are summarized in §5.

## 1. Results against the acceptance criteria (Round 2)

| criterion | result | evidence |
|---|---|---|
| translated headers equal to the originals, or the diff explained | Parts are **byte-identical**. The only differing line in each header is the cell, which is now a struct over the fold instead of an alias of `hapi::APIOf` (`round2/log/cell.diff`) | built |
| `check/build.sh` unchanged | 61 ok, 0 FAIL on the translated tree, same as the original tree. The two outputs are identical line for line. This covers the host tests (g++, clang++), the 6 must-not-build programs, the AVR sizes, the 4 identical-disassembly checks, and simavr row by row (`bitexact.py`: BIT-EXACT) | host, built, simulated |
| Banknote `wave4` | **44 B flash, 0 B RAM, 274/274, 25 cycles** (min = mean = max), net of the null program (`compare_emlearn/run.sh` + `report.py`). `lin4` is identical to the original too (112 B, 0 B, 273/274, 62 cycles) | simulated |
| `objdump` over all 27 AVR programs | **27/27 raw-identical** `avr-objdump -d`: the 15 from `build.sh`, `bnc` wave4/lin4, and all 10 of `measure/`. The fallback comparison with symbol names stripped was never needed: everything that touches a cell type is inlined, so no symbol mentions `wave::Cell<…>`. `measure/` cycles are identical too (`wave4:23 lin4:33`) | built, simulated |

The two cell lines, as translated:

```c++
template<u8 k,typename... OO> struct Cell : (OO : ... : Bias<k> : API) {};
// ->
template<u8 k,typename... OO> struct Cell : hapi::Chain<OO...,Bias<k>>::template Part<API> {using Base=typename hapi::Chain<OO...,Bias<k>>::template Part<API>; using Base::Base;};
```

`lin::CellOf` gets the same treatment (terminal `APIOf<Acc>`). `lin::Cell=CellOf<acc,b,OO...>` stays a plain alias, which is allowed because it has no `:`.

### Checks that depend on matching `APIOf<...>` by pattern
No check failed. These are the places that name `APIOf`, and why each one still passes:

| place | what it matches | why it still passes |
|---|---|---|
| `include/sugar.h` `cell()` | builds the **rolled** cell as `hapi::APIOf<lin::API,R,Roll<...>,lin::Bias<b>>` and the **unrolled** one as `lin::Cell<b,R,TT...>` | `sugar.h` is not translated: the rolled cell is still an `APIOf`, and the unrolled cell is whatever `lin::Cell` now is |
| `check/sugar_roll.cpp:17` | `is_same_v<U<decltype(six)>, hapi::APIOf<lin::API,...>>` (the rolled cell) | as above. If `sugar.h` were rewritten in struct form, this assert would have to name that struct instead: it pins the `APIOf` pattern |
| `check/sugar_roll.cpp:16,22,25,30` | `is_same_v<..., lin::Cell<...>>` (unrolled) | both sides are `lin::Cell`, which now names the struct |
| `check/sugar_roll_hand_avr.cpp` (and `same` in `build.sh` against `sugar_roll_avr`) | the "by hand" rolled net spelled `hapi::APIOf<...>` | the sugar side is still `APIOf`, so the disassembly is identical |
| `models/roll60/roll_common.h` | `using C=hapi::APIOf<API32,OO...>` | not translated, and has its own terminal |
| `refid.h` / `refid_*`, `refq_check`, `net_expand` | `FromTypes<Q>` reads a cell's `Types`; `Expand<Net>` holds the cells whole | the struct inherits `Types` from `Chain::Part`: `Chain<OO...,Bias<k>>`, **without the API**. No static_net query asks for the API, and matching tags and parts (`Tag<id>`, `lin::Sign`, `wave::Threshold`) still works *(host, built)* |

`round2/cell_vs_apiof.cpp` pins the difference on both compilers *(built)*:
- `wave::Cell<k,OO...>` is not `hapi::APIOf<API,OO...,Bias<k>>`.
- It has the same base.
- Its `Types` is `Chain<OO...,Bias<k>>`, where `APIOf`'s is `Chain<API,OO...,Bias<k>>`.

What static_net silently no longer gets from `APIOf` on these two cells, none of which it exercises:
- the `BuildRules` static_assert (no static_net part declares rules)
- the `Expand<APIOf>` / `HasOwnRules<APIOf>` specializations (a cell nested inside another rule-validated container)
- `APIOf::Part<T>`

A mixed net of struct cells and `APIOf` cells (sugar's rolled cells) works *(host: `sugar_check`, `sugar_roll`)*.

## 2. What worked

- **The restricted grammar is enough for static_net.** It needs open classes (the seven parts of the two engines) plus a named struct over a pack fold for the cell. *(built)*
- **A small Python script is enough, with no Clang LibTooling.** Edits are spliced onto the original text, so comments and whitespace survive. *(built)*
- **Round 1, 5/5:** `struct My : Twice:Id {};` runs on g++ and clang++. Bare use of `Twice` is rejected, and `using My = Twice:Id;` is refused. *(host, built)*
- **Round 3, 71/71:**
  - Base-clause chains work, including an access specifier next to an ordinary base, and folds (empty pack included).
  - Named compositions match their plain-C++ equivalents (`named_chain.cpp`), including a dependent base (`template<class T> struct W : B:T`).
  - Constructor inheritance through a chain *and* into the named struct needs no user `using`: `Offset(int)` hides `Term(int)`, while `Term(int,int)` is still inherited.
  - An open rightmost operand ends in `od::Nil` and runs, while its unused `super` call stays unchecked (`open_terminal.cpp`, also for a fold with an empty pack).
    Calling that member is a compile error (`neg_compile/open_terminal_super.cpp`: g++ `'missing' is not a member of 'B::Part<od::Nil>::Base' {aka 'od::Nil'}`, clang++ `no member named 'missing' in 'od::Nil'`).
  - `Add<1>:Add<2>:Term` is two layers (`distinct_args.cpp`).
  - Rule 2 holds: family, not subtype, with separate statics.
  - Rule 3 holds nominally, across TUs.
  - Rule 7 holds: user `super` precedence, and dependence.
  - The label hazard is pinned.
  - All of the above on g++ and clang++. Also under experimental `--lower=nested`. *(host)*
  - **Diagnostics:** 13 translator refusals (rules 4, 6, 7, 8 and 9, duplicate layers, and scope) and 5 compiler rejections. *(built)*

## 3. Semantics as implemented

- **Rule 3 (identity) holds trivially:** a composition's identity is its name. The same struct is the same type in every TU (`identity_tu1/2` link) *(host)*.
  - Two structs over the same chain are different types with the same base and behaviour: `struct G : A:(B:C)` and `struct X : A:B:C` have `G::Base == X::Base` *(host)*.
- **Rule 1 (injected-class-name):** the translator rewrites the class's own name inside an open class's body to `Part`. That makes it the family-member *layer*, which is a base of the named struct, not the struct itself (`self.cpp`, asserted in both modes) *(host)*.
  - `A<other args>` is not rebound, which is correct: it names another specialization.
  - Macros are not seen.
- **"super undefined": any compiler error is accepted.** For reference *(built)*:

  | situation | g++ | clang++ |
  |---|---|---|
  | bare use `Twice::f(21)` | `'f' is not a member of 'Twice'` | `no member named 'f' in 'Twice'` |
  | `struct Bad : Twice:Empty {}`, used | `'f' is not a member of 'Twice::Part<Empty>::Base' {aka 'Empty'}` | `no member named 'f' in 'Empty'` |
  | `A& r = ab;` (rule 2) | `... from expression of type 'AB'` | `... unrelated type 'AB'` |

  Named compositions print as their own name (`AB`), where the alias form printed `hapi::Chain<A>::Part<B>`.
- **Rule 4 is now base clause only.** The hazard stays as supporting evidence: `A:B x;` in a block is valid C++ today (label `A:` then `B x;`). It compiles and `x` is a `B` *(host, `label_hazard.cpp`)*.
- **Rule 6** becomes "every struct with a `:` base inherits that base's constructors", lowered to `using Base=...; using Base::Base;`.
  - A class with two `:` bases is refused: `Base` would be ambiguous.
  - A class with *ordinary* bases next to one `:` base is fine.
- **Rule 7 (open vs closed)** is decided at the definition, by whether the body names `super`.
  - A closed class as a left operand is refused.
  - A data-only mixin must write `using super::super;` to be usable as `M:X`.
  - The proposal should state this. Rule 1's "any A" and rule 7's "only the last layer may be closed" conflict without it.
  - With the no-explicit-termination amendment the last layer may be either: closed (it is the terminal) or open (the chain
    ends in `od::Nil`). Left operands must still be open.
- **Rule 8 (rebasing) is unchanged.**
  - An open class with an ordinary base is refused.
  - So is a named composition used as a *left* operand, and `(A:B):C`.
  - A named composition can be the **last** operand: `struct Y : Any:X {}`.
- **Open terminal** (amendment). An open rightmost operand becomes the last layer over `od::Nil`. Its `super`
  calls are dependent: checked only when used, and then a compile error naming `od::Nil` *(host, built)*.
  - Detected textually: the operand names a class defined in the same file whose body names `super`. An open class from
    another file used as the terminal is lowered as a plain terminal, and its `super` calls then fail to compile when the class is used, since a
    holder has no members of its own. This is a translator limitation, not part of the rule.
- **Duplicate layers** (amendment). `A:X` is refused when `A` already occurs among the operands to its right, or in their bases. The bases are followed recursively
  through named classes defined in the file (`:` bases and ordinary ones):
  - `struct Y : Self:Self:Term {}`: `[od-dup] duplicate layer: 'Self' occurs twice in the composition`
  - `struct X : A:C {}; struct Y : A:X {};`: `[od-dup] duplicate layer: 'A' is composed over 'X', which already derives from 'A' (via 'X')`

  The match is on exact spelling after removing whitespace, so `Bias<1>:Bias<2>` is legal *(built)*.
  Not seen: pack elements (`OO...` is only known at instantiation), aliases (`Wave<...>` vs `WaveOf<Slot<...>,...>`), and
  equal types spelled differently (`Bias<1>` vs `Bias<0+1>`). A compiler, or a HAPI uniqueness rule run at instantiation,
  would check the exact types, including packs.
- **Rule 9 (fold)**:
  - `(T : ... : PP)` and `(PP : ... : T)` have the same token shape. The translator uses the template head to find the pack, and only the right fold exists.
  - An unparenthesized fold in a base clause is refused.
- **`typename` / `::template`** are added in dependent contexts, both in the base clause and in the injected `using Base=typename ...;`.

## 4. What is lost compared with the alias form

1. **Every composition needs a name.** There is no anonymous `A:B` at a use site. `Twice:Id` can't be written inline in a template argument.
2. **Two structs over the same chain are different types** (`struct Y : Any:X {}` vs `struct Y2 : Any:A:B:C {}`), with the same behaviour.
   With aliases, identity was structural and could hold across independently written code. Now it is nominal, so shared compositions need a shared name.
3. **A named composition cannot be a left operand** (rule 8), only the last operand. Composing compositions means ending a chain at one: `Any:X` gives `Chain<Any>::Part<X>`, not a flat `Chain<Any,A,B>::Part<C>`.
4. **The injected-class-name inside a part means the layer, not the named struct** (§3, rule 1).
5. **`APIOf`'s extras are not carried:**
   - `Types` with the API first
   - `BuildRules` validation
   - `Expand` / `HasOwnRules`
   - `Part<T>`

   A struct over a fold gets only what `Chain::Part` has (`Types` without the API) and the injected `Base`. static_net needed none of the rest (§1).
6. **Generic partial application needs a struct template** (`template<class T> struct AnyOf : Any:T {};`) instead of an alias template. Each instantiation is again its own named type.

The experimental `--lower=nested` (`A::Part<B::Part<C>>`, `od::FoldT`) has no `Chain` wrapper, so a struct over it has no `Types`.
Through `check/build.sh` it gives **46 ok / 15 FAIL**: every refid/`RefQ`/`net_expand` check fails ("no cell in the net matches Q"). Everything that does not query components still builds on avr-gcc 7.3 at the same sizes (`round2/variants/nested/build.txt`) *(host, built)*.
The wrapping lowering is the one HAPI's introspection needs.

## 5. Superseded: identity results of the alias form (earlier round)

With `using X = A:B:C;` allowed:
- `Any:X` and `Any:A:B:C` lowered to different types (`Chain<Any>::Part<Chain<A,B>::Part<C>>` vs `Chain<Any,A,B>::Part<C>`), with the same behaviour.
- Making them equal (the nested lowering) cost HAPI's `Types`.

The struct-only form removes the question: an alias can no longer carry a composition, and identity is nominal.

## 6. Future work

- **Fallback-API sugar.**
  - The problem: with no explicit termination, a chain that ends open gets `od::Nil`, an empty terminal. So a `super::proc(in)` that
    reaches the end is a compile error.
  - What `APIOf` does instead: it provides a fallback API at the end (`wave::API`: `proc` returns 0, `update` does nothing), and every chain ends on it by hand
    (`(OO : ... : Bias<k> : API)`).
  - The idea: let a family declare its fallback API once, so that an open chain of that family ends on it instead of `od::Nil`.
    Then `struct Cell : (OO : ... : Bias<k>) {};` would need no explicit `: API`.
  - To be designed: where the declaration lives (on the part, on a family tag), how it combines across mixed chains, and
    what the lowering is (the translator would pick the terminal where it now picks `od::Nil`).
- **Duplicate layers in packs and through aliases**, checked on exact types at instantiation (see §3). This is a candidate HAPI rule, next to
  the uniqueness rules `APIOf`'s `BuildRules` already runs.

## 7. Suggested HAPI improvements (tools that are missing)

- **Validation for named compositions.** A struct over a bare `Chain::Part` skips `BuildRules`/`NoCollision`, which `APIOf` runs.
  - A small HAPI helper that a named composition can opt into would restore it: `hapi::Validated<Chain<...>>`, or a `static_assert` the translator could inject next to `using Base::Base;`.
  - A hook for this could also add the API-first `Types`, if queries on the terminal are ever needed.
- **A non-wrapping fold in `chain.h`**, as in `support/od_fold.h`. Only needed if the nested lowering is ever wanted. It is not needed by the struct-only form.

## 8. Translator limitations

- Its classification is textual:
  - Openness comes from `super` tokens.
  - Dependence comes from template-parameter names in scope.
  - Class-name checks are file-local and by simple name, so a closed class defined in another file is not caught as a left operand; the compiler catches it later.
- Macros are opaque.
- There is no semantic lookup.
- Names the lowering introduces are refused where they would be captured: `O`, `Base` and `Part` inside open classes, and `Base` inside named compositions.
- The out-of-line check is a heuristic: a declared-only member function in an open class is refused, and that could misfire on a function-like macro at class scope.
- The translator does not add includes: `<hapi/chain.h>`, `od_nil.h` when a chain ends open, or `od_fold.h` for experimental nested folds.
- Open-terminal and duplicate-layer detection only see classes defined in the file being translated (§3).
- Round-3 programs use `<cstdio>`/`<type_traits>` and are host-only. The lowered forms they exercise (dependent `typename ...::template Part` in a base clause and in the injected `Base`) build with avr-gcc 7.3 in round 2 *(built)*.
