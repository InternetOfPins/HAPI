# Open Derivation: findings from the translator prototype

Base commit: `5c4bfeb`. Toolchains: g++ and clang++ (host, `-std=c++17`), avr-gcc 7.3.0, avr-binutils, simavr 1.6
(ATmega328p @ 16 MHz), all from Ubuntu 24.04 packages.

**Evidence levels:**
- **built:** compiles, or is rejected with the stated diagnostic.
- **host:** runs natively and the checks hold.
- **simulated:** runs in simavr.
- **silicon:** runs on a real Nano. None in this round: simavr has matched silicon at 0 cycles difference on this example (`examples/static_net/measure/silicon_results.md`).

Logs are in each round's `log.txt` (round 2 also has `log/`).

**How this document is organized:**
- **Part I** is the language as proposed, stated without any library names.
- **Part II** is how the prototype lowers it to HAPI, a library that already implements the same idea in C++17. The mechanics there
  (`APIOf`, `Expand`, `Distinct`, ...) are the prototype's, not the language's.
- **Part III** is the evidence. Part II's lowering reproducing existing hand-written code (static_net) byte for byte, with identical machine code, is evidence *for* the rules in Part I.
- **Parts IV and V** cover the HAPI core addition and the limitations.

---

## Part I. The language

1. **Late derivation.** `A:B` is the class `A` with its definition re-declared over the base `B`: the base is chosen where `A` is used,
   not where it is defined.
   - **Family, not subtype.** `A:B` is "an `A`" the way `vector<int>` is "a vector".
     - It is not bare `A` and does not derive from it; it derives from `B`.
     - Each family member (`A:B`, `A:C`) has its own static members.
   - Inside `A`'s body, `A` means the family member (`A:B`), not bare `A`.
2. **Where `:` may appear:** only in a base clause: `struct Z : A:B:final T {...};`.
   - Anywhere else (an alias-declaration, a declaration in a block) it is not derivation.
   - In a block it already has a meaning today: `A:B x;` is the label `A:` followed by `B x;`.
3. **Layers and the terminal.**
   - **Layers.** Every operand without `final` is a layer, whatever its body says. `A:B:C` is right-associative: `A` over `B` over `C`.
   - **The terminal (`final T`).** Written on the last operand, it makes `T` the chain's most-base class: nothing is added below `T`.
   - **Contrast with the existing meaning of `final`.** In `struct X final : B`, `final` means nothing may derive from `X`, so the hierarchy stops *above* `X`.
     In `: final T`, it stops *below* `T`. It is a contextual keyword, and inside a base clause it is followed by a type. `final` alone, `final::X` or `final<...>` name a type called `final`.
4. **Closed and partial classes.**
   - `struct Z : A:B:final T {...};` is an ordinary, complete class, whose members are its body. Its identity is its name, the same in every translation unit.
   - A chain without `final` (`struct W : A:B {};`) is a **partial class**:
     - It is usable only as a layer (`struct Z : W:C:final T {}`) and cannot be instantiated.
     - It has no members of its own; members belong in a layer.
     - It is how compositions are built on the fly, and it is closed where it is used.
   - `struct Z : final T {}` is closed on `T` with no layers.
5. **`super`.** Inside a class, `super` names the base that class gets at the point of use.
   - If ordinary lookup already finds a `super` (the old `typedef Base super;` idiom), that one wins.
   - Expressions naming `super` are dependent: they are checked when the member is used in a concrete composition, not at the class's definition.
   - A class that names `super` is usable only as a layer; used bare, the lookup fails. A class that does not name `super` is usable both bare and as a layer, because its body does not depend on a base.
   - Inside a closed class, `super` names its base (`struct A : B:final C {..}` means the same as `struct C {..}; struct B : C {..}; struct A : B {..};`).
6. **Constructors.** Every layer, and every closed class, inherits its base's constructors, transitively along the chain. A layer's constructor with the same signature hides the inherited one.
7. **Pack fold.** A right fold over types: `template<class... PP> struct Z : (PP : ... : P : final T) {};`. It is parenthesized, and the pack comes first.
8. **Ill-formed:**
   - a duplicate layer (the same type twice in one composition, through packs, aliases and partial classes too), like a duplicate direct base today (`struct X : N, N {}`)
   - `final` on any operand but the last
   - a closed class as a layer (it is complete, and adding below it would change it)
   - rebasing a class that already has a base (`struct A : Y {}`, then `A` as a layer)
   - `(A:B):C` (a composition as a left operand)

**Open questions** (not settled by the prototype):
- **Members in a partial class.** The prototype refuses them. Natively they could become a layer of their own.
- **Two `:` bases in one class** (`struct Z : A:final T, B:final U`). The prototype refuses this for lowering reasons (Part II). Natively it is ordinary multiple inheritance, and only `super` would be ambiguous.

## Part II. Lowering to HAPI: existing practice

HAPI (`include/hapi/`) is a C++17 library in which components are written as `template<class O> struct Part : O {...}` holders, a `Chain<...>` of them
is an open composition, and `APIOf<API, ...>` closes it on a terminal API and collapses the Parts into one class. The IOP libraries
(OneMenu's `ItemDef<OO...>`, `NavDef<II...>`) build on it with **XXXDefs**: structs derived from `APIOf` with a fixed API. `translate.py` lowers
Part I onto exactly these forms.

| language (Part I) | lowering (prototype mechanism) | native meaning |
|---|---|---|
| a class that names `super` | `struct A {template<class O> struct Part:O {using Base=O; using Base::Base; ...Base::...};};` | a class whose base is supplied at use |
| a class that does not name `super`, used as a layer | kept as is, plus `template<class O> struct Part:O {...same body...};` appended | the same class, usable bare and as a layer |
| `super` | `Base` (the `Part`'s base alias) | the base supplied at use |
| the class's own name in its body | `Part` (the holder's member template) | the family member |
| `final T` closing `A:B` | `struct Z : hapi::APIOf<T,A,B>`: `T` first; `APIOf` starts the Part collapse | the chain's most-base class is `T`; nothing is added below it |
| a chain without `final` | `struct W : hapi::Chain<A,B> {}`, body refused | a partial class: only a layer, cannot be instantiated |
| constructor inheritance | `using Base=...; using Base::Base;` in the closed struct (as `APIOf` itself does) | inherited constructors |
| duplicate layer is ill-formed | `static_assert(hapi::Distinct<hapi::Chain<layers...,T>>, "duplicate layer in Z")` | ill-formed, like a duplicate direct base |
| components' constraints | `APIOf`'s rule walk (`BuildRules` over each component's `rules<Before,After>()`); `rules()` stays on the class | library traits (no native counterpart needed) |
| — | after each closed struct: `using Z_APIOf=hapi::APIOf<...>;` then `hapi::Expand` / `HasOwnRules` entries forwarding to it (XXXDef style) | nothing; introspection comes from reflection or library traits |
| pack fold | `hapi::APIOf<T,PP...,P>`, or `hapi::Chain<PP...,P>` without `final` | the fold |
| `:` outside a base clause | refused (`[od-rule4]`) | not derivation |

**Notes on the lowering:**
- **Duplicate layers are not a C++ error once lowered.** `A:A:T` is `A::Part<A::Part<T>>`, two different types, which compile silently. So `hapi::Distinct`
  (added to HAPI core, Part IV) stands in for the native error, on exact types.
  - Rejected in round 3, on g++ and clang++, with `duplicate layer in Z|Y` *(built)*:
    - `Self:Self`
    - through a pack (`Z<A,A>`)
    - through an alias (`Wave<0>` vs `WaveOf<Slot<0>>`)
    - `Bias<1>` vs `Bias<0+1>`
    - `A` over a partial class that already holds `A`, from the same file and from another header
  - It also caught a real mistake in round 3's own `base_chain.cpp` (`ZF<Inc,Dbl,Inc>`).
- **Why XXXDef entries are needed:** HAPI keys `Expand` / `HasOwnRules` on the exact type, and a closed struct is derived from `APIOf`, not an `APIOf`.
  - The documented one-line forwarding (`meta.h`) goes through the `<Name>_APIOf` alias, declared where the operands' names resolve. `<Name>::Base` would instantiate the composed class on every probe, which `rules.h` avoids for `APIOf`.
  - Non-type template parameters become `auto`.
  - The Def is named from the root (`::a::b::X`), so a global `Nil` is not captured by `hapi::Nil`.
  - The enclosing namespaces are closed and reopened with their own openers. This works for the global namespace, `namespace a::b`, inline and anonymous namespaces (`def_namespaces.cpp`).
  - A Def nested in a class gets no entry, with a warning, and stays a leaf.
- **Why closing is `APIOf<T,...>` rather than `Chain<...>::Part<T>`:** in HAPI, `APIOf` is what closes. It runs the rule walk, and its `Types` lists the API first,
  which HAPI's queries read. The closed struct derives from exactly the `APIOf` a hand-written HAPI program would name.
- **Prototype-only restrictions** (lowering reasons, not language rules):
  - two `:` bases in one class (one injected `Base`)
  - members defined out of line (a `.cpp`) in a layer class (the `Part` copy cannot carry them)
  - a `super` inside `rules()` (`rules()` stays on the class, which has no base)

## Part III. Evidence

### Round 2: static_net (the acceptance criteria)

`examples/static_net/include/waveCell.h` and `linCell.h`, rewritten in `:` syntax and translated:

| criterion | result | evidence |
|---|---|---|
| translated headers equal to the originals, or the diff explained | The seven layers are **byte-identical**. The only differing line in each header is the cell: `struct Cell : (OO : ... : Bias<k> : final API) {};` becomes a struct deriving from **exactly the `hapi::APIOf` the original alias named**, plus its XXXDef entries (`round2/log/cell.diff`) | built |
| `check/build.sh` unchanged | 61 ok, 0 FAIL on the translated tree, identical output to the original tree. That covers the host tests (g++, clang++), 6 must-not-build programs, AVR sizes, 4 identical-disassembly checks, and simavr row by row (`bitexact.py`: BIT-EXACT) | host, built, simulated |
| Banknote `wave4` | **44 B flash, 0 B RAM, 274/274, 25 cycles** (min = mean = max), net of the null program. `lin4` is identical to the original (112 B, 0 B, 273/274, 62 cycles) | simulated |
| `objdump` over all 27 AVR programs | **27/27 raw-identical** `avr-objdump -d` (`build.sh`'s 15, `compare_emlearn`'s wave4/lin4, `measure/`'s 10). `measure/` cycles are identical too (`wave4:23 lin4:33`) | built, simulated |

`round2/cell_vs_apiof.cpp` *(built)*: `wave::Cell<k,OO...>` is its own type, derives from `hapi::APIOf<API,OO...,Bias<k>>`,
and has the same `Types` (API first), the same `Expand` children and policy, and the same `HasOwnRules`.

**Checks that match `APIOf<...>` by pattern.** None failed.
- `sugar.h` builds its rolled cells as `hapi::APIOf<...>` directly (untranslated), and `check/sugar_roll.cpp:17` / `sugar_roll_hand_avr.cpp` match that spelling. If `sugar.h` were rewritten in `:` syntax, that assert would name the Def instead.
- The unrolled cells compare `lin::Cell` with `lin::Cell`.
- `models/roll60/roll_common.h` is untranslated.
- refid/`RefQ`/`net_expand` read `Types`, which is unchanged.

### Round 1 (5/5)
`struct My : Twice:final Id {};` runs on g++ and clang++. Bare use of `Twice` is rejected, and `using My = Twice:final Id;` is refused *(host, built)*.

### Round 3 (84/84)
18 programs on g++ and clang++, the nested-class warning, 15 translator refusals, and 16 compiler rejections on both compilers *(host, built)*.

- **Part I rule 1** (family, statics, rebinding): `family.cpp` and `closed_layer.cpp`, with separate statics per family member and for the bare class, and `self.cpp`.
- **Rule 2:**
  - `label_hazard.cpp`: `A:B x;` in a block compiles as a label.
  - `alias_form.cpp`: the alias form is refused.
- **Rule 3:**
  - `final_type.cpp` / `final_layer.cpp`: a type named `final` as terminal (`A:final final`), as a layer, and as a partial class's end.
  - `neg_translate/final_not_last.cpp`.
- **Rule 4:**
  - `component.cpp`: a partial class, one nested in another, a partial fold, each closed later on a user terminal, and `final T` with no layers.
  - `neg_translate/component_body.cpp`.
  - `neg_translate/closed_as_layer.cpp`.
  - `identity.cpp`, `identity_tu1/2.cpp`: nominal identity across TUs.
- **Rule 5:**
  - `named_chain.cpp`: `super` in a closed class against its plain-C++ equivalent, with a dependent base.
  - `user_super.cpp`: user `super` precedence.
  - `dependent.cpp`.
  - `closed_layer.cpp`: a data-only mixin as a layer, no `using super::super` needed; a class with a constructor; a class with `rules()`; closed classes from another header.
  - `neg_compile/bare_use.cpp`, `super_undefined.cpp`, `component_super.cpp`: any compiler error is accepted. g++ says `'f' is not a member of 'Twice'`, clang++ `no member named 'f' in 'Twice'`.
- **Rule 6:** `ctors.cpp`: inherited through the chain and into the closed class; `Offset(int)` hides `Term(int)`, while `Term(int,int)` is still inherited.
- **Rule 7:** `base_chain.cpp`, `rules_ok.cpp`, `fold_left.cpp`, `fold_unparenthesized.cpp`.
- **Rule 8:**
  - `neg_compile/dup_*`: six duplicate cases.
  - `neg_translate/rebase_*`: rebasing refused.
- **Lowering:**
  - `def_expand.cpp` / `neg_compile/def_nested_rules.cpp`: a Def nested in an outer rule walk is spliced like its `APIOf`, and a component rule fires through it. A hand-written derived struct without the entry is a silent leaf.
  - `def_namespaces.cpp`: the namespace forms.
  - `neg_compile/rules_*`: component `rules()` rejecting compositions (`B only after A`), and a `rules()` that answers `false` (`HAPI: validation failed`).
  - `neg_translate/out_of_line_*`: layer classes with members defined out of line, open and closed.

### What the evidence says about the language
- **Existing practice matches Part I.** The seven static_net layers, hand-written in HAPI's Part form, are exactly what Part I's layers lower to, byte for byte.
  The two cells are exactly what Part I's closed classes lower to. Machine code, sizes, cycles and accuracy are unchanged on all 27 AVR programs.
- **No compiler tooling was needed.** The restricted grammar (base clause only, `final` on the last operand) is simple enough that a ~1000-line Python script translates it.

## Part IV. HAPI core: `hapi::Distinct<L>`

- **Where:** `include/hapi/rules.h` (commit `2d3be50`), with tests in `tests/compile_tests.cpp` (g++, clang++, avr-gcc 7.3), a `docs/REFERENCE.md` row, and `CHANGELOG.md`.
- **Impact:** it is additive, and HAPI's own tests are unchanged: `compile_tests` passes, and `tests/negative` is 17/17 on g++ and clang++.
- **How it compares:** it flattens the list, then compares on exact types.
  - Nested `Chain`s are spliced.
  - Types with `::Types` (a partial class, an `APIOf`, a Def) are replaced by their `Types`, recursively.
  - Types with `Part<O>` are layers, compared by `is_same`.
  - Everything else is a terminal, compared by `is_same`, and by `is_base_of` either way with the other terminals.
- **It is the only guard.** The translator's earlier spelling-based duplicate check was removed (`22347aa`), because the compiler rejects all it caught, and also what it missed.
- **Deferred HAPI decision: `Distinct` inside `APIOf` itself.** Enforcing it there would break a hand-written composition that repeats a layer on purpose. Skipping it leaves hand-written HAPI unchecked.
  - A middle way: make it **opt-out through the rules system**. Every `APIOf` is checked by default, and a composition that means to repeat a layer declares so.
  - This waits until a real case of intentional repetition turns up (OneMenu or elsewhere).

**Fallback-API sugar** needs nothing new: a Def fixes a family's terminal once (`template<class... OO> struct CellDef : (OO : ... : final API) {};`,
as OneMenu's `ItemDef<OO...>` fixes `ItemAPI`), and users write `CellDef<A,B>`.

## Part V. Translator limitations

- **Classification is textual.**
  - Whether a class names `super` is read from its tokens.
  - Dependence comes from template-parameter names in scope.
  - Macros are opaque, and there is no semantic lookup.
- **Which classes are used as layers is collected by simple name, across all the files of one invocation.** Two consequences:
  - A closed class used as a layer only in a file outside the batch gets no `Part`, and the compiler reports it at the use.
  - The collection over-approximates: an unrelated class with the same simple name also gets a `Part` (round 3: `family.cpp`'s `B`).
    That `Part` is never instantiated and has no codegen effect.
  - static_net's classes that are never layers (`API`, `Features`, `lin::APIOf`) are untouched, which the byte-identical round trip shows.
- Names the lowering introduces (`O`, `Base`, `Part`) are refused where they would be captured.
- The out-of-line check is a heuristic: a member function that is declared but not defined in a layer class is refused. That could misfire on a function-like macro at class scope.
- The translator does not add includes: the input includes `<hapi/hapi.h>`.
- Round-3 programs use `<cstdio>`/`<type_traits>` and are host-only. The lowered forms build with avr-gcc 7.3 in round 2 *(built)*.

**History:**
- An earlier round accepted the alias form (`using X = A:B;`). Structural identity then clashed with HAPI's introspection. That form was dropped for the struct-only form.
- The implicit terminal `od::Nil` and the `--lower=nested` mode were dropped when closing became explicit (`final`) and `APIOf`-based.
- Rule 7's "a class that does not name `super` cannot be a layer" was an artifact of inferring the terminal. It was dropped once `final` marked the terminal.
