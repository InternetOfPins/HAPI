# Open Derivation: findings from the translator prototype

Base commit: `5c4bfeb`. Toolchains: g++ and clang++ (host, `-std=c++17`), avr-gcc 7.3.0, avr-binutils, simavr 1.6
(ATmega328p @ 16 MHz), all from Ubuntu 24.04 packages.

**Evidence levels:**
- **built:** compiles, or is rejected with the stated diagnostic.
- **host:** runs natively and the checks hold.
- **simulated:** runs in simavr.
- **silicon:** runs on a real Nano. None in this round: simavr has matched silicon at 0 cycles difference on this example (`examples/static_net/measure/silicon_results.md`).

Logs are in each round's `log.txt` (round 2 also has `log/`).

## 0. The form of the proposal these results are for

- **Syntax is the surface, HAPI is underneath.** Whether a composition is closed, and on what, is written in the source. The translator
  never infers it from what an operand happens to be.
- **`:` only in a base clause:** `struct Z : A:B:final T {...};`, and pack folds `struct Z : (OO : ... : P : final T) {};`.
  - The alias form `using X = A:B;` is refused, `[od-rule4]`.
- **`final T` closes the composition on the terminal API `T`.** It is always the last operand, and `T` is the user's (their own `Nil` when there is no API).
  - It lowers to `hapi::APIOf<T,layers...>` (`T` first, as `APIOf`'s first parameter), because in HAPI `APIOf` is what closes: it starts the Part collapse.
  - `struct Z : final T {}` is `APIOf<T>`, closed with no layers.
- **Without `final`, the chain stays open: a component.** `struct W : A:B {};` lowers to `hapi::Chain<A,B>`.
  - A component is reusable as a layer (`struct Z : C:W:final Nil {}`) and is closed where it is used. That is how components are built on the fly.
- **A closed composition is an XXXDef**, the IOP style (OneMenu's `ItemDef<OO...>`, `NavDef<II...>`): a struct derived from `APIOf`, with
  `using Base=...; using Base::Base;` and its own `hapi::Expand` entry. The translator emits the entry (§3).
- **A closed composition's body is the composed object.** `super` names its base, and it inherits that base's constructors (`using Base=...; using Base::Base;` is injected, as `APIOf` itself does).
  - `struct A : B:final C {..}` ⇔ `struct C {..}; struct B : C {..}; struct A : B {..};` *(host, `named_chain.cpp`)*.
- **Checks are HAPI's, run by the compiler.**
  - `APIOf` runs HAPI's rule walk, i.e. the components' `rules<Before,After>()`.
  - Every composed struct also gets `static_assert(hapi::Distinct<...>)` for duplicate layers.
  - A component's rules are only checked once it is closed, because only then are its Before/After known.
- **HAPI core changes in one place:** `hapi::Distinct` in `include/hapi/rules.h` (§6).
- **Dropped along the way:**
  - the implicit `od::Nil` terminal (closing is explicit now)
  - `--lower=nested` (the collapse must start at `APIOf`)
  - the translator's own duplicate check and rule walk (HAPI does both)

## 1. Results against the acceptance criteria (Round 2)

| criterion | result | evidence |
|---|---|---|
| translated headers equal to the originals, or the diff explained | Parts are **byte-identical**. The only differing line in each header is the cell: a struct closed with `final API`, whose base is **exactly the `hapi::APIOf` the original alias named** (`round2/log/cell.diff`) | built |
| `check/build.sh` unchanged | 61 ok, 0 FAIL on the translated tree, same as the original tree. The two outputs are identical line for line. This covers the host tests (g++, clang++), the 6 must-not-build programs, the AVR sizes, the 4 identical-disassembly checks, and simavr row by row (`bitexact.py`: BIT-EXACT) | host, built, simulated |
| Banknote `wave4` | **44 B flash, 0 B RAM, 274/274, 25 cycles** (min = mean = max), net of the null program (`compare_emlearn/run.sh` + `report.py`). `lin4` is identical to the original too (112 B, 0 B, 273/274, 62 cycles) | simulated |
| `objdump` over all 27 AVR programs | **27/27 raw-identical** `avr-objdump -d`: the 15 from `build.sh`, `bnc` wave4/lin4, and all 10 of `measure/`. `measure/` cycles are identical too (`wave4:23 lin4:33`) | built, simulated |

The two cell lines:

```c++
template<u8 k,typename... OO> struct Cell : (OO : ... : Bias<k> : final API) {};
// ->
template<u8 k,typename... OO> struct Cell : hapi::APIOf<API,OO...,Bias<k>> {using Base=hapi::APIOf<API,OO...,Bias<k>>; using Base::Base;
  static_assert(hapi::Distinct<hapi::Chain<OO...,Bias<k>,API>>, "duplicate layer in Cell");};
```

`lin::CellOf` gets the same treatment (`final APIOf<Acc>`). `lin::Cell=CellOf<acc,b,OO...>` stays a plain alias, which is allowed because it has no `:`.
`round2/cell_vs_apiof.cpp` pins it on both compilers *(built)*: `wave::Cell<k,OO...>` is its own type, derives from exactly `hapi::APIOf<API,OO...,Bias<k>>`,
and has the same `Types` (API first). It also has the same `Expand` children and policy and the same `HasOwnRules` as that `APIOf`. `APIOf`'s rule walk runs as before.

### Checks that depend on matching `APIOf<...>` by pattern
No check failed. The places that name `APIOf`, and why each still passes:

| place | what it matches | why it still passes |
|---|---|---|
| `include/sugar.h` `cell()` | builds the **rolled** cell as `hapi::APIOf<lin::API,R,Roll<...>,lin::Bias<b>>` and the **unrolled** one as `lin::Cell<b,R,TT...>` | `sugar.h` is not translated: the rolled cell is still an `APIOf`, and the unrolled cell is whatever `lin::Cell` now is |
| `check/sugar_roll.cpp:17` | `is_same_v<U<decltype(six)>, hapi::APIOf<lin::API,...>>` (the rolled cell) | as above. If `sugar.h` were rewritten in struct form, this assert would have to name that struct instead: it pins the `APIOf` pattern |
| `check/sugar_roll.cpp:16,22,25,30` | `is_same_v<..., lin::Cell<...>>` (unrolled) | both sides are `lin::Cell`, which now names the struct |
| `check/sugar_roll_hand_avr.cpp` (and `same` in `build.sh` against `sugar_roll_avr`) | the "by hand" rolled net spelled `hapi::APIOf<...>` | the sugar side is still `APIOf`, so the disassembly is identical |
| `models/roll60/roll_common.h` | `using C=hapi::APIOf<API32,OO...>` | not translated |
| `refid.h` / `refid_*`, `refq_check`, `net_expand` | `FromTypes<Q>` reads a cell's `Types` | the struct inherits `APIOf`'s `Types`, API first, the same list as before *(host, built)* |

Nothing that `APIOf` gives is lost: the XXXDef entries (§3) forward `Expand` / `HasOwnRules`, which HAPI keys on the exact type. A mixed net of struct cells and `APIOf` cells (sugar's rolled cells) works *(host: `sugar_check`, `sugar_roll`)*.

## 2. What worked

- **The restricted grammar is enough for static_net.** It needs open classes (the seven parts of the two engines) plus one closed struct over a pack fold per engine. *(built)*
- **A small Python script is enough, with no Clang LibTooling.** Edits are spliced onto the original text, so comments and whitespace survive. *(built)*
- **Round 1, 5/5:** `struct My : Twice:final Id {};` runs on g++ and clang++. Bare use of `Twice` is rejected, and `using My = Twice:final Id;` is refused. *(host, built)*
- **Round 3, 84/84:** 17 programs on g++ and clang++, the nested-class warning, 17 translator refusals, and 16 compiler rejections on both compilers. *(host, built)*
  - **Components (`component.cpp`):**
    - A component (`W : A:B`), a component of a component (`W2 : C:W`), and a component fold (`(PP : ... : B)`) all work.
    - Each is closed where it is used, on the user's own `Nil` or on another terminal: the same `W` is closed twice, as `W:final Nil` and as `W:final T`.
    - `final T` alone works.
    - `B`'s unused `super::missing()` compiles. Calling it is a compile error naming the user's `Nil` (`neg_compile/component_super.cpp`).
  - **Base-clause chains and folds:** including an access specifier next to an ordinary base, and an empty pack (`APIOf<Term>`).
  - **Named compositions** match their plain-C++ equivalents, including a dependent base (`template<class T> struct W : B:final T`).
  - **Constructor inheritance** through a chain and into the closed struct needs no user `using`: `Offset(int)` hides `Term(int)`, while `Term(int,int)` is still inherited.
  - **Rule 2 holds:** family, not subtype, with separate statics.
  - **Rule 3 holds nominally,** across TUs.
  - **Rule 7 holds:** user `super` precedence, and dependence.
  - **The label hazard is pinned.**

## 3. Semantics as implemented

- **Closing (`final T`).**
  - **What `final` means here, next to its existing meaning:** in `struct X final : B`, `final` says nothing may derive from `X`. It closes
    the hierarchy *upward*, above `X`. In `(OO : ... : final API)`, it marks the end of the chain *downward*: nothing goes below `API`, and the
    composition's terminal is fixed. Both are "the hierarchy stops here", on opposite sides.
  - **Why it is needed, not just a closed rightmost operand:**
    - Without it, whether the last operand closes the composition or is a component's open end would have to be inferred from that class's body. That is only possible for classes defined in the same file.
    - With it, the syntax says so, a component can end on an open class, and closing is explicit.
  - **Parsing.** `final` is only a contextual keyword, so a type named `final` is legal C++ (`struct final {};`). In an operand, `final` is the
    keyword only when a type follows it in that operand. `final` alone, `final::X` and `final<...>` name the type called `final`.
    - `: final API` stays unambiguous: in a base clause a type name directly followed by another type name has no other parse. And `final`
      is only valid right after a class name today, never inside a base-specifier.
    - **Round 3** (`final_type.cpp`, `final_layer.cpp`, `neg_translate/final_type_open_end.cpp`), g++ and clang++ *(host, built)*:
      - `A:final final` closes on a type named `final` (`APIOf<final,A>`), and so do `final final` (`APIOf<final>`), `W:final final` and a fold.
      - An *open* class named `final` works as a layer (`A:final:final T` is `APIOf<T,A,final>`) and as a component's open end (`A:final` is `Chain<A,final>`).
      - A *closed* type named `final` as a component's open end is refused, with the hint to write `final final`.
  - `[od-final]` refusals:
    - `final` not on the last operand
    - a closed class used as the open end of a component: "close the composition on it with `final K`"
- **Components.** A chain without `final` lowers to `hapi::Chain<layers...>`. A component is a type list, so members in its body would not be part of any
  composed object, and a non-empty body is refused (`[od-component]`). It is checked once it is closed:
  - `APIOf`'s rule walk sees the component spliced in place (`Expand<Chain>` validates).
  - `Distinct` splices its `Types`.
- **Rule 8 (rebasing) and closed compositions.**
  - A *closed* composition cannot be a layer (`[od-rule8] 'X' is a closed composition`). Compose the component instead.
  - A closed composition can be the terminal: `struct Y : Any:final X {}`.
  - An open class with an ordinary base, and `(A:B):final C`, are still refused.
- **XXXDef entries.** HAPI's walks key `Expand` / `HasOwnRules` on the exact type, and a closed struct is derived from `APIOf`, not an `APIOf`. So
  right after each closed struct the translator emits the one-line forwarding that HAPI's `meta.h` documents for types derived from `APIOf`,
  as OneMenu's `ItemDef` has. Round 2's `Cell`, as translated:
  ```c++
  template<u8 k,typename... OO> using Cell_APIOf=hapi::APIOf<API,OO...,Bias<k>>; } namespace hapi {
    template<auto k,typename... OO> struct Expand<wave::Cell<k,OO...>> : Expand<wave::Cell_APIOf<k,OO...>> {};
    template<auto k,typename... OO> struct HasOwnRules<wave::Cell<k,OO...>> : HasOwnRules<wave::Cell_APIOf<k,OO...>> {}; } namespace wave {
  ```
  - **Why the `<Name>_APIOf` alias:** it names the base where the operands' names resolve (inside `wave`). Forwarding through `Cell<...>::Base`
    would instantiate the whole composed class whenever a walk probes it, which `rules.h` avoids on purpose for `APIOf`.
  - **Non-type parameters become `auto`:** their types may be names local to the struct's namespace (`u8`).
  - **Namespaces:** the enclosing namespaces are closed and reopened around the entry. Each namespace brace is recorded with its own opener
    text, closed with one `}`, and reopened with the same opener. The Def is named from the root (`::a::b::X`), so no name in `namespace hapi` captures it.
    - **Tested** (`def_namespaces.cpp`, g++ and clang++; each Def expands as its `APIOf`) *(host, built)*:
      - **global namespace:** nothing to close. The Def here is named `Nil`, which `hapi::Nil` would capture without the root qualification.
      - **`namespace a::b`:** the nested definition has one brace, closed with one `}` and reopened as `namespace a::b {`.
      - **inline namespace (`namespace v { inline namespace v1 {`):** reopened with `inline`, and the Def is a class template.
      - **anonymous namespace, at the top and inside a named one:** reopening `namespace {` in the same translation unit reopens the same unnamed
        namespace. The Def gets no qualifier from it and is found through its implicit using-directive.
  - **Where it cannot be emitted:** a closed struct nested in a class or a block gets no entry. It stays a leaf for HAPI's walks, and the translator prints a warning,
    which `round3/run.sh` checks (`warning: 'In' is nested in a class or block: no hapi::Expand entry`).
  - **Round 3 (`def_expand.cpp`, `neg_compile/def_nested_rules.cpp`)** *(host, built)*:
    - `Z : C:final T` expands as `APIOf<T,C>` does: it validates, and its children are `T,C`.
    - Nested in an outer rule walk (`BuildRules<Chain<>,Chain<Z,D>>`), `C`'s rule "no D after C" fires.
    - The same derivation written by hand without the entry (`struct Bare : hapi::APIOf<T,C> {}`) is a leaf, and the rule never sees the `D`.
  - **Round 2:** avr-gcc 7.3 accepts the entries (every AVR build of `build.sh` includes the headers), and codegen is unchanged: 27/27 identical *(built)*.
- **Rule 3 (identity) holds trivially:** a composition's identity is its name. The same struct is the same type in every TU (`identity_tu1/2` link) *(host)*.
  - Two structs over the same chain are different types with the same base and behaviour: `struct G : A:(B:final C)` and `struct X : A:B:final C` have `G::Base == X::Base` *(host)*.
- **Rule 1 (injected-class-name):** inside an open class's body, the class's own name is rewritten to `Part`, the layer. That is a base of the closed struct, not the struct itself (`self.cpp`) *(host)*.
- **"super undefined": any compiler error is accepted.** For reference *(built)*:

  | situation | g++ | clang++ |
  |---|---|---|
  | bare use `Twice::f(21)` | `'f' is not a member of 'Twice'` | `no member named 'f' in 'Twice'` |
  | `struct Bad : Twice:final Empty {}`, used | `'f' is not a member of 'Twice::Part<Empty>::Base' {aka 'Empty'}` | `no member named 'f' in 'Empty'` |
  | `A& r = ab;` (rule 2) | `... from expression of type 'AB'` | `... unrelated type 'AB'` |

- **Rule 4 is base clause only.** The hazard stays as supporting evidence: `A:B x;` in a block is valid C++ today (label `A:` then `B x;`). It compiles and `x` is a `B` *(host, `label_hazard.cpp`)*.
- **Rule 6:** a closed struct inherits its base's constructors.
  - A class with two `:` bases is refused (`Base` would be ambiguous).
  - Ordinary bases next to one `:` base are fine.
- **Rule 7 (open vs closed class)** is decided at the definition, by whether the body names `super`.
  - A closed class as a layer is refused. It can only be the terminal, `final K`.
  - A data-only mixin must write `using super::super;` to be a layer.
- **Duplicate layers: left to the compiler. Status: done.** `hapi::Distinct` is in HAPI core (`include/hapi/rules.h`, commit `2d3be50`), and every composed struct emits its
  `static_assert`. The translator's spelling-based check was removed in `22347aa`: the compiler rejects everything it caught, and also what it missed.
  - Natively, a duplicate layer is the same kind of error as a duplicate direct base (`struct X : Nil, Nil {}`: g++ `duplicate base type 'Nil' invalid`).
    In the Part lowering it is not one: `A:A:T` is `A::Part<A::Part<T>>`, two different types, which would compile silently.
  - So every composed struct gets `static_assert(hapi::Distinct<hapi::Chain<layers...,T>>, "duplicate layer in Z")`, the prototype's stand-in for that native error. How it works is in §6.
  - **Rejected** (`neg_compile/`, g++ and clang++, `duplicate layer in Z|Y`; clang++ prints the flattened list) *(built)*:
    - `Self:Self:final Term`
    - via a pack: `Z<A,A>` (`Chain<A, A, T>`)
    - via an alias: `Wave<0>` vs `WaveOf<Slot<0>>`
    - `Bias<1>` vs `Bias<0+1>` (`Chain<Bias<1>, Bias<1>, T>`)
    - `A` over a component `X : A:B`, from the same file (`dup_layer.cpp`) and from another header (`dup_other_header.cpp`)
  - **It caught a real mistake:** round 3's own `base_chain.cpp` had instantiated `ZF<Inc,Dbl,Inc>`, and the test now uses `ZF<Dbl,Inc>`.
  - **Not seen:** layers inside a closed operand that has no `Types`, e.g. a hand-written `struct K : A::Part<C> {}`, since `A::Part<C>` is not `A`.
- **Component rules (HAPI's rules system), run by `APIOf`.** A component's `rules<Before,After>()` is asked of the holder, so the translator keeps a
  `rules` member of an open class outside `Part`, verbatim. `super` inside `rules()` is refused, because the holder has no base.
  - **Round 3** (`rules_ok.cpp`, `neg_compile/rules_*`), g++ and clang++ *(host, built)*:
    - `A:B:final T` passes, as does `ZF<A,B>` through a pack. `hapi::APIOf<T,A,B>` agrees.
    - `B:final T`, `B:A:final T` and `ZF<B,A>` fail with the component's own message, "B only after A".
    - A `rules()` that answers `false` fails with `APIOf`'s own "HAPI: validation failed".
- **Rule 9 (fold)**:
  - `(T : ... : PP)` and `(PP : ... : T)` have the same token shape. The translator uses the template head to find the pack, and only the right fold exists.
  - An unparenthesized fold is refused.
  - A closed fold ends in `final T`. A component fold does not.
- **No `typename` / `::template` in the output any more.** `hapi::APIOf<...>` and `hapi::Chain<...>` are template-ids at namespace scope, not dependent member templates.

## 4. What is lost, and what is kept, compared with the alias form

1. **Every composition needs a name.** There is no anonymous `A:B` at a use site.
2. **Two structs over the same chain are different types,** with the same behaviour. Identity is nominal, so shared compositions need a shared name.
3. **Only components compose further.** A closed composition is a terminal, never a layer. Build the component, then close it where it is used.
4. **The injected-class-name inside a part means the layer,** not the named struct.
5. **`APIOf`'s extras are kept** (`Types` with the API first, the rule walk, `Part<T>`), because the closed struct *is* an `APIOf` by derivation.
   The exact-type `Expand`/`HasOwnRules` entries are forwarded (XXXDef style, §3).
6. **Generic partial application needs a struct template** (`template<class T> struct AnyOf : Any:final T {};`), not an alias template.

## 5. Future work

- **Fallback-API sugar is the XXXDef itself.** A family fixes its terminal API once in a Def, as OneMenu's `ItemDef<OO...>` fixes `ItemAPI`:
  `template<class... OO> struct CellDef : (OO : ... : final API) {};` is used as `CellDef<A,B,...>`, and nobody spells the terminal again.
  No new syntax is needed. static_net's `Cell<k,OO...>` is already such a Def (its bias is a parameter).
- **`Distinct` inside `APIOf`?** It would give hand-written HAPI compositions the same check. It is not done, because it changes existing HAPI behaviour:
  a composition that repeats a layer on purpose would stop compiling.

## 6. HAPI core: `hapi::Distinct<L>`

- **Where:** `include/hapi/rules.h`, with tests in `tests/compile_tests.cpp` (g++, clang++, avr-gcc 7.3), an entry in `docs/REFERENCE.md`, and `CHANGELOG.md`.
- **Impact:** it is additive. HAPI's own tests are unchanged: `compile_tests` passes on g++ and clang++, and `tests/negative` is 17/17 on both.
- **How it compares:** it flattens the list, then compares on exact types.
  - Nested `Chain`s are spliced.
  - A type with `::Types` (a component struct, an `APIOf`, a closed struct) is replaced by its `Types`, recursively.
  - Types with `Part<O>` are open layers, compared by `is_same`.
  - Everything else is a closed operand, compared by `is_same`, and by `is_base_of` either way with the other closed operands.
- **Why the translator passes the operand list, not `Base::Types`:** it names the terminal and the packs directly, and an empty closed fold (`APIOf<T>`) has no layers of its own.

## 7. Translator limitations

- Its classification is textual:
  - Openness of a class comes from `super` tokens.
  - Class-name checks (closed class as a layer, component or closed composition) are file-local and by simple name.
    A misuse involving a class from another file is caught later by the compiler, or not at all when it only changes meaning.
- Macros are opaque.
- There is no semantic lookup.
- Names the lowering introduces are refused where they would be captured: `O`, `Base` and `Part` inside open classes, and `Base` inside closed compositions.
- The out-of-line check is a heuristic: a declared-only member function in an open class is refused, and that could misfire on a function-like macro at class scope.
- The translator does not add includes: the input includes `<hapi/hapi.h>`, for `Chain`, `APIOf` and `Distinct`.
- Round-3 programs use `<cstdio>`/`<type_traits>` and are host-only. The lowered forms (`hapi::APIOf<T,OO...,P>` in a template's base clause) build with avr-gcc 7.3 in round 2 *(built)*.
