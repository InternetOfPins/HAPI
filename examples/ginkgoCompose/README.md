# ginkgoCompose

A concrete look at the `apply()` dispatch cost that
[Ginkgo](https://github.com/ginkgo-project/ginkgo)'s own TOMS-2022 paper
describes in §7.2 ("The cost of runtime polymorphism"): one `apply()`
traverses three polymorphism forks — format, executor, kernel-variant
selection. This example fixes the executor as a template parameter so
forks 2 and 3 resolve at compile time, keeps the class a drop-in
`gko::LinOp`, and measures what's gone (objdump) and what it cost you
(the runtime executor knob).

No GPU, no `nvcc`. Reference executor only.

## What this does and does not show

This started as a "can HAPI interface Ginkgo" experiment. A Ginkgo
maintainer ([@yhmtsai](https://github.com/ginkgo-project/ginkgo/discussions))
reviewed it and was right on two points, both folded in here:

1. **No metaprogramming library is needed.** Selecting the kernel by a
   compile-time executor type is a one-line template specialization
   (`StencilKernel<Exec>`). An earlier version routed it through
   `hapi::Chain<>` + `FindFirst<>`; that produced **byte-identical**
   `apply_impl` codegen (verified — same 170 instruction bytes), so it
   was removed from `stencil.cpp`. `advdiff.cpp` still uses a
   `hapi::Chain<>::Part` layer stack, but for two terms plain data
   members are equivalent — it's shown because it takes N
   independently-defined terms without editing any of them, not because
   it's necessary.
2. **The comparison changes the requirements.** Ginkgo's
   `Executor::run` vtable exists to translate a *runtime* choice
   (`./program cuda`) into its already-compile-time-typed internal
   kernels. Fixing `Exec` at compile time removes the dispatch by
   removing that capability — it is not "the same thing, faster". A
   format author who only ever builds single-backend can bind the
   kernel directly anyway.

So: this is a demonstration of the cost *structure* and the trade-off,
not a proposed improvement to Ginkgo, and not a case where HAPI earns its
keep over plain C++.

## Build

```sh
./setup_ginkgo.sh     # one-time: clones + builds reference-only Ginkgo (~430 MB)
./verify.sh           # plain g++: builds every variant, shows the objdump, runs it
```

PlatformIO (`GINKGO_DIR` via `extra_ginkgo.py`):

```sh
pio run -e stencil_baseline     # Ginkgo's custom-matrix-format example
pio run -e stencil_fixed        # executor fixed at compile time
pio run -e advdiff_combination  # gko::Combination{cd·L + ca·D}
pio run -e advdiff_prefold      # user pre-folds into one Csr
pio run -e advdiff_hapi         # fused compile-time layer composition
```

## Sources

| file | default | `-DFIXED_EXEC` / `-DPREFOLD` | `-DHAPI` |
|---|---|---|---|
| `src/stencil.cpp` | `StencilMatrix` — Ginkgo's [`custom-matrix-format`](https://github.com/ginkgo-project/ginkgo/tree/develop/examples/custom-matrix-format) (Listing 7), reference trim | `StencilMatrixCT<VT, Exec>` — `Kernel = StencilKernel<Exec>`, plain specialization | — |
| `src/advdiff.cpp` | `gko::Combination<double>{cd, Csr(L), ca, Csr(D)}` | `-DPREFOLD`: one folded `Csr` `[cd−ca, −2cd, cd+ca]` | `AdvDiffHapi` — `hapi::Chain<DiffLayer,AdvLayer>::Part<LinOpTerminal>`, each layer its own `gko::array` subobject |

## Result — the objdump (`./verify.sh`)

`apply_impl(const LinOp*, LinOp*)`, every `call` / relocation in the body:

**`StencilMatrix` (baseline):**
```
gko::as<Dense>(const*)        downcast
gko::as<Dense>(*)             downcast
__libc_single_threaded        branch for the atomic below
lock addl 8(%r12)             shared_from_this refcount bump
vtable for ...stencil_operation   build the Operation on the stack
call *0x30(%rdx)              INDIRECT — Executor::run vtable   (fork 2)
_Sp_counted_base::_M_release  x2   shared_ptr teardown
__stack_chk_fail              stack canary
_Unwind_Resume                exception landing pad
```
…then, in libginkgo, `ReferenceExecutor::run` does 2 logger callbacks +
`op.run(...)` — a second virtual dispatch (fork 3).

**`StencilMatrixCT` (executor fixed):**
```
gko::as<Dense>(const*)        downcast
gko::as<Dense>(*)             downcast
```
That's the whole list. The stencil is inlined straight into `apply_impl`;
no `Operation` type is ever built. The two `gko::as` downcasts stay
(interface-inherent). Fork 1 (`apply()` → virtual `apply_impl`) stays —
that's what keeps it drop-in.

`AdvDiffHapi::apply_impl`: `gko::as` ×2 + one `memset` (zero-init); both
layer contributions inlined, no virtual, no `Operation`.

## Result — numbers (`./verify.sh`, `./statrun.sh`)

Reference executor, gcc 13, `-O2`, AMD FX-8350, no root (`schedutil`
governor). All variants: numerics bit-identical, zero heap allocations
per `apply()`.

**Stencil** — 1-D Poisson solve avg rel. error `7.08e-13` either way.
Bare-`apply()` timing (size-11, paper §7.2 method), paired interleaved
(`statrun.sh`): the paired delta `baseline − fixed` is always positive
and tight within a run, but shifts with thermal/load state — ~16 ns
idle-cold, ~30 ns under sustained load. Representative: baseline ~140–160
ns/apply, fixed ~123–133, **~15–30 ns removed** (forks 2+3 plus the
`shared_ptr`/canary/unwind baggage). Hardware- and state-dependent — the
objdump is the load-bearing result.

**advdiff** — `A = cd·L + ca·D`, n=32:

| approach | ns / `apply()` | operand types fixed at compile time? |
|---|---|---|
| `gko::Combination` (2 Csr) | ~6800 | no (runtime `shared_ptr<LinOp>`) |
| pre-folded single `gko::Csr` | ~2070 | yes |
| `AdvDiffHapi` fused | **~285** | yes |

`gko::Combination` is a `LinOp` whose `apply_impl` fans out to one full
`LinOp::apply` per operand. Pre-folding into one `Csr` is still ~7× the
fused version — a general `Csr` carries its own internal dispatch stack.
As with the stencil case, the fused version trades away runtime operand
composition (`Combination`'s actual purpose) for the speed.

## Scope

- Forks 2+3 are **host-side, paid before kernel launch** — identical on
  reference / OpenMP / CUDA / HIP, and independent of problem size.
  Negligible for large GPU SpMV; a real fraction only for many small
  `apply()`s (batched/block solvers, strong-scaling limits).
- No change to Ginkgo `core/` is proposed. `LinOp`/`Executor` do real
  work (runtime format and executor choice, heterogeneous containers,
  logging hooks) that fixing types at compile time removes.
