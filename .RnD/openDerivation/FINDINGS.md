# Open Derivation: findings from the translator prototype

Base commit: `5c4bfeb`. Toolchains: g++ and clang++ (host, `-std=c++17`), avr-gcc 7.3.0, avr-binutils, simavr 1.6
(ATmega328p @ 16 MHz), all from Ubuntu 24.04 packages.

**Evidence levels:**
- **built:** compiles, or is rejected with the stated diagnostic.
- **host:** runs natively and the checks hold.
- **simulated:** runs in simavr.
- **silicon:** runs on a real Nano. None in this round: simavr has matched silicon at 0 cycles difference on this example (`examples/static_net/measure/silicon_results.md`).

Logs are in each round's `log.txt` (round 2 also has `log/`).

## 1. Results against the acceptance criteria (Round 2)

| criterion | result | evidence |
|---|---|---|
| translated headers equivalent to the originals | **byte-identical**: `waveCell.h` and `linCell.h` (`cmp`), and so token- and comment-equal (`tokdiff.py`) | built |
| `check/build.sh` unchanged | 61 ok, 0 FAIL on the translated tree, same as the original tree. The two outputs are identical line for line. This covers the host tests (g++, clang++), the 6 must-not-build programs, the AVR sizes, the 4 identical-disassembly checks, and simavr row by row (`bitexact.py`: BIT-EXACT) | host, built, simulated |
| Banknote `wave4` | **44 B flash, 0 B RAM, 274/274, 25 cycles** (min = mean = max), net of the null program, from `compare_emlearn/run.sh` + `report.py`. `lin4` is identical to the original too (112 B, 0 B, 273/274, 62 cycles) | simulated |
| `objdump` identical to the original build | 27/27 AVR programs have identical `avr-objdump -d`: the 15 from `build.sh`, `bnc` wave4/lin4, and all 10 of `measure/`. `measure/` cycles are identical too (`wave4:23 lin4:33`) | built, simulated |

The headers came out byte-identical, so the equal disassembly is expected. It is checked anyway, and the check needs no whitespace allowance.

## 2. What worked

- **The restricted grammar is enough for static_net.** Only two constructs were needed: an open class, meaning a class that names `super`, and the pack fold. All seven parts of the two engines are open classes (`Bias`, `WaveOf`, `Threshold`, `Store`, `BiasOf`, `TermOf`, `SignOf`). No self-reference, no out-of-line members, no rebasing. *(built)*
- **A small Python script is enough.** No Clang LibTooling was needed. The lexer and bracket matcher are about 700 lines of Python, and edits are spliced onto the original text so comments and whitespace survive. *(built)*
- **Round 1:** `using My = Twice:Id;` runs on g++ and clang++. Bare use of `Twice` is rejected. *(host, built)*
- **Round 3, 53/53:**
  - Chains in base clauses work, including one with an access specifier next to an ordinary base, and folds in base clauses (empty pack included).
  - Rule 6 constructor inheritance works through a chain, and `Offset(int)` hides the inherited `Term(int)` while `Term(int,int)` is still inherited.
  - Rule 2 holds: family, not subtype. `A:B` converts to `B&`, is not an `A`, and `A:B`/`A:C` have separate statics.
  - Rule 3 holds across translation units: a function taking `X&` with `X = A:B:C` links between two TUs in both modes.
  - Rule 7 lookup precedence holds: `typedef Base0 super;` and `using super = Base0;` classes are left alone and work.
  - Rule 7 dependence holds: a member using a `super::missing()` that no base has is fine until it is called.
  - All of the above in both lowering modes, g++ and clang++. *(host)*
- **Diagnostics:** 9 translator diagnostics (rules 7, 8 and 9, and scope) and 4 compiler rejections, each checked for its message. *(built)*

## 3. Deviations of the lowering from the semantics

### D1. `--lower=chain` breaks rule 3 (structural identity). *(host)*
`hapi::Chain<A,B>::Part<C>` is a *struct* deriving from `A::Part<B::Part<C>>`, not an alias. So with `X = A:B:C`:
`Any:X` lowers to `Chain<Any>::Part<Chain<A,B>::Part<C>>`, and `Any:A:B:C` lowers to `Chain<Any,A,B>::Part<C>`.
These are two different types, and `static_assert(!std::is_same<...>)` holds on both compilers (`round3/src/pos/identity.cpp`).
The translator flattens *syntactic* right-grouping, so `A:(B:C)` == `A:B:C` holds. It cannot flatten through an alias or a template parameter.

`--lower=nested` (`A::Part<B::Part<C>>`, and `od::FoldT` for packs) restores rule 3 exactly: `Any:X` == `Any:A:B:C` *(host)*.
But see D2.

### D2. Exact identity and HAPI's introspection are in tension. *(host, built)*
HAPI finds a cell's components through the `Types` member of the `Chain` wrapper (`FromTypes`, `RefId`/`RefQ`, `FindFirst`).
The nested lowering has no wrapper, so it has no `Types`. With `Cell` as a nested fold, `check/build.sh` gives
**46 ok / 15 FAIL**: every refid/`RefQ`/`net_expand` check fails ("no cell in the net matches Q")
(`round2/variants/cell_fold/build_nested.txt`). The chain lowering passes 61/61.
Everything that does not query components still builds on avr-gcc 7.3 at the same sizes (e.g. `sugar_avr` 274 B / 4 B, `sonar_lin_avr_size` 1842 B / 61 B).

So an `A:B:C` that is exactly its structure cannot, today, also be asked what it is made of. The proposal puts
"deducing X from A:X" out of scope. But HAPI needs the component list, and today it gets it from a wrapper, which costs rule 3.

### D3. Rule 1 (injected-class-name rebinding) is implemented, not a known deviation. *(host)*
The translator rewrites the class's own name inside the body when it is unqualified and has no template arguments: `A`, `A::x`, and constructors `A(...)`.
It becomes `Part`, the injected-class-name of `A::Part<O>`. Constructors need this anyway to compile.

With `--lower=nested` the name means exactly `A:B`, so `is_same<X::Me, X>` holds. With `--lower=chain` it means the `Part` *under* the Chain wrapper,
which is a base of `X` but not `X` (`round3/src/pos/self.cpp`, both asserted).

What is left:
- `A<other args>` inside the body is not rebound, which is correct: it names another specialization.
- Uses of the name inside macros are not seen.
- A local variable that shadows the class name would be rewritten.

static_net parts never name themselves, so none of this touches round 2.

### D4. "super undefined" is not the diagnostic you get. *(built)*
Rule 7 promises a plain "super undefined" lookup error on bare use. The lowering gives:

| situation | g++ | clang++ |
|---|---|---|
| bare use `Twice::f(21)` | `'f' is not a member of 'Twice'` | `no member named 'f' in 'Twice'` |
| `Twice:Empty` formed and used | `'f' is not a member of 'Twice::Part<Empty>::Base' {aka 'Empty'}` | `no member named 'f' in 'Empty'` |
| `A& r = ab;` (rule 2) | `... of type 'AB' {aka 'hapi::Chain<A>::Part<B>'}` | `... unrelated type 'AB' (aka 'Part<B>')` |

The errors are correct and early, but they talk about the holder (bare `Twice` is an empty struct) and spell types
in the lowered form. A real implementation would say `super` and print `A:B`. The translator cannot do better
without either changing the holder, which would break byte-identity with the HAPI headers, or doing semantic analysis.

### D5. `typename` / `::template` are needed in templates; the brief's table omits them. *(built)*
In a dependent context C++17 requires `typename hapi::Chain<OO...,Bias<k>>::template Part<API>` in an alias RHS, and `::template` in a base clause.
The translator adds these keywords when an operand names a template parameter that is in scope, for example
`round2/variants/cell_fold/out/waveCell.h`, which builds with g++, clang++ and avr-gcc 7.3.

### D6. Rule-7 lookup precedence is textual.
- A class declaring its own `super` (member `typedef`/`using`) is left alone *(host)*.
- A namespace-scope `super` switches `super` to ordinary lookup for the rest of the file. This is judged by position and ignores namespace nesting (a warning is printed).
- `super` inside macros is invisible.

Rule 8 makes the check almost local: an open class has no base of its own, so a base-class member named `super` cannot exist.

## 4. What the proposal should change or state

1. **Say which lowering rule 3 means, and pick the identity model.** If `A:(B:C)`, `A:B:C` and `Any:X` (X = A:B:C) must all be one type (D1), then:
   - A chain can't be a wrapper type.
   - Introspection needs another source (D2).

   Options:
   - (a) The language gives `A:B` a component list that can be queried, which partly un-scopes "deducing X from A:X".
   - (b) Each layer describes itself (a HAPI change, §5).
   - (c) Rule 3 is weakened to "same operand sequence, same type", which is what `--lower=chain` gives.
2. **Reconcile rule 1 with rule 7 on closed classes.** Rule 1 says *any* class `A` has an `A:B`. Rule 7 says only the last layer may be closed.
   The prototype follows rule 7:
   - Whether a class is open is decided at its definition, by whether it names `super`.
   - A closed class as a left operand is an error (`[od-rule7] 'K' is closed`).
   - A data-only mixin must write `using super::super;` to be usable as `M:X`.

   Either state that openness is a property of the definition, and name its marker (`super`, or an explicit one), or allow closed left operands.
   A translator would then need the use sites to know which classes to lower, or would have to emit both forms.
3. **State the rule-4 hazard explicitly.** `A:B x;` in a block is valid C++ today: the label `A:` followed by `B x;`.
   It compiles and `x` is a `B` (`round3/src/pos/label_hazard.cpp`, host, both compilers). Rule 4 is not only about avoiding
   ambiguity: outside the two allowed contexts, the syntax already has a silent, different meaning.
4. **Define how a fold finds its pack.** `(T : ... : PP)` and `(PP : ... : T)` have the same token shape. Telling them apart needs the knowledge that
   `PP` is a pack. That is trivial for a compiler, but the text should say that only the right fold (pack first) exists, as rule 9 implies.
5. **Named wrappers: the requirement is weaker than stated, at least for static_net.**
   `template<typename API, typename... OO> struct APIOf : (OO : ... : API) {};` lowers to exactly the base of `hapi::APIOf`
   (static_asserts, host). But writing `Cell` as a *bare fold* instead of `hapi::APIOf` still passes `build.sh` 61/61, with 9/9 identical
   disassembly and the same wave4/lin4 rows *(host, built, simulated; `round2/variants/`)*. The reason is that `Chain::Part` carries its own `Types`.
   What the named wrapper adds, and static_net does not exercise:
   - the API in `Types`
   - `BuildRules` validation
   - the `Expand<APIOf>` specialization

   "Decomposition stays on named wrappers" holds only as long as the chain lowering (D1) is kept.
6. **Constructors through a chain need nothing new.** Rule 6 lowered to `using Base::Base;` per layer, plus the one in `Chain::Part`, works transitively. It also works with hiding by a same-signature constructor that initializes `super(...)` *(host)*.

## 5. Suggested HAPI improvements (tools that are missing)

- **A non-wrapping fold in `chain.h`**, for example `Chain<OO...>::template Fold<T>` = `O1::Part<...On::Part<T>>` as an alias.
  This is what `support/od_fold.h` provides for the prototype. With it, a composition can be exactly its structure when it wants to be (rule 3), without leaving HAPI.
- **Self-describing layers**, which would resolve D2 without a wrapper.
  - The idea: if the Part form carried its own component list, for example `using Types = typename Base::Types::template App<Holder>`, defaulting to `Chain<T>` at the terminal, then refid/`FromTypes` could read a nested composition directly.
  - Why the prototype doesn't do it: it would add a line to every part, so the byte-identity check of round 2 would need a new baseline.
  - **Status:** not implemented, and worth a round of its own.
- **Name-collision checks for bare compositions.** A bare `Chain::Part` (and so a bare fold) skips `BuildRules` and `NoCollision`.
  The bare-fold `Cell` loses validation silently. A static_net `Cell` has no rules today, so nothing broke, but the gap is real.

## 6. Translator limitations

- Its classification is textual:
  - Openness comes from `super` tokens.
  - Dependence comes from template-parameter names in scope.
  - Class-name checks are file-local and by simple name, so a closed class defined in another file is not caught as a left operand; the compiler catches it later.
- Macros are opaque.
- There is no semantic lookup.
- Refused, with messages:
  - out-of-line members of open classes, including a declared-only member function (a heuristic that could misfire on a function-like macro at class scope)
  - rebasing
  - closed left operands
  - left or unparenthesized folds
  - `super` outside a class
- Names the Part lowering introduces (`O`, `Base`, `Part`) are refused inside open classes and as their template parameters.
- `--lower=nested` with packs needs `#include "od_fold.h"`. The translator does not add includes.
- The output of `AVR` programs in round 3 was not checked. The lowered forms it uses (dependent `typename ...::template Part`, `od::FoldT`) build with avr-gcc 7.3 in round 2 and its variants *(built)*.
