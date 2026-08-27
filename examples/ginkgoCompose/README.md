# ginkgoCompose

A drop-in [Ginkgo](https://github.com/ginkgo-project/ginkgo) `gko::LinOp`
whose executor and kernel-variant selection are resolved at compile time
by a HAPI `Chain<>` instead of Ginkgo's runtime polymorphism — proving
HAPI composes a real, non-trivial polymorphic C++ interface (sparse
linear algebra) as cleanly as it already composes DSP pipelines
(`OneHLS`), parse pipelines (`OneParse`), and spatial indexing
(`cutlass_layout`).

Ginkgo's own TOMS-2022 paper (§7.2, "The cost of runtime polymorphism")
states that one `apply()` call traverses **three polymorphism forks** —
format selection, executor selection, kernel-variant selection. This
example removes forks 2 and 3 while keeping the class a drop-in
`gko::LinOp`: still `create()`-able, still goes straight into
`cg::build()...->generate(...)->apply(...)`.

No GPU, no `nvcc`, no CUDA toolkit. Reference executor only.

## Build

```sh
./setup_ginkgo.sh            # one-time: clones + builds a reference-only
                             # Ginkgo (~430 MB) at ~/ginkgo-src
./verify.sh                  # plain g++: builds every variant, shows the
                             # objdump fork structure, runs correctness + timing
```

Or with PlatformIO (`GINKGO_DIR` picked up by `extra_ginkgo.py`):

```sh
pio run -e stencil_baseline        # Ginkgo's own custom-matrix-format example
pio run -e stencil_hapi            # drop-in HAPI variant
pio run -e advdiff_combination     # gko::Combination{cd·L + ca·D}
pio run -e advdiff_prefold         # user pre-folds into one Csr
pio run -e advdiff_hapi            # fused compile-time composition
```

## What each source does

| file | default | `-DPREFOLD` | `-DHAPI` |
|---|---|---|---|
| `src/stencil.cpp` | `StencilMatrix` — Ginkgo's [`examples/custom-matrix-format`](https://github.com/ginkgo-project/ginkgo/tree/develop/examples/custom-matrix-format) (Listing 7), reference-executor trim | — | `StencilMatrixHapi` — `gko::Operation`'s per-executor `run()` overload set becomes `hapi::Chain<RefStencil,OmpStencil>`, picked by `hapi::FindFirst<ServesExec<Exec>>` at compile time |
| `src/advdiff.cpp` | `gko::Combination<double>{cd, Csr(L), ca, Csr(D)}` | one folded `Csr` `[cd−ca, −2cd, cd+ca]` | `AdvDiffHapi` — body `hapi::Chain<DiffLayer,AdvLayer>::Part<LinOpTerminal>`, each layer a real `gko::array` subobject, one fused pass |

## Result — the objdump (`./verify.sh`)

`apply_impl(const LinOp*, LinOp*)`, every `call`/relocation in the body:

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

**`StencilMatrixHapi` (drop-in):**
```
gko::as<Dense>(const*)        downcast
gko::as<Dense>(*)             downcast
```
That's the whole list. The stencil is inlined straight into `apply_impl`.
`hapi::Chain` + `FindFirst` leave **no symbol and no relocation** —
resolved entirely at compile time. The two `gko::as` downcasts are kept
in both (interface-inherent). Fork 1 (`apply()` → virtual `apply_impl`)
is kept by design — that is what "drop-in" means.

`AdvDiffHapi::apply_impl`: `gko::as` ×2 + one `memset` (zero-init); both
layer contributions inlined, no virtual, no `Operation`.

## Result — numbers (`./verify.sh`, `./statrun.sh`)

Reference executor, gcc 13, `-O2`, AMD FX-8350 (no root — `schedutil`
governor). All variants: **bit-identical numerics, zero heap allocations
per `apply()`.**

**Round 2** — 1-D Poisson solve: avg rel. error `7.08e-13`, both.
Bare-`apply()` timing (size-11 matrix, paper §7.2 method), paired
interleaved (`statrun.sh` — the paired difference cancels the DVFS this
box can't be told to disable). The paired delta `base − hapi` is **always
positive and tightly clustered within a run**, but the cluster shifts
with machine thermal/load state (~16 ns idle-cold, ~30 ns after sustained
load) — no fixed governor available. Representative: baseline ~140–160
ns/apply, HAPI ~123–133, **~15–30 ns removed**.

The nanoseconds are hardware- and state-dependent; **the objdump is the
load-bearing result.** The removed cost is forks 2+3 plus the
`shared_ptr`/canary/unwind baggage the `Operation` struct pulls in.

**Round 3** — advection-diffusion `A = cd·L + ca·D`, n=32:

| approach | ns / `apply()` | keeps L, D separate? |
|---|---|---|
| `gko::Combination` (2 Csr) | ~6800 | yes |
| pre-folded single `gko::Csr` | ~2070 | no |
| `AdvDiffHapi` fused | **~285** | yes (2 layers) |

`gko::Combination` is itself a `LinOp` whose `apply_impl` fans back out to
one *full* `LinOp::apply` per operator. Pre-folding into one `Csr` is
still 7× slower than the fused composition — a general `Csr` drags its
whole internal dispatch stack. So the point is **not** "faster than
Ginkgo": it is that HAPI keeps `L` and `D` independently meaningful
(reuse, retune coefficients) *without* the per-operator `apply()` tax.

## Scope / honesty

- Forks 2+3 are **host-side dispatch, paid before kernel launch** —
  identical on reference / OpenMP / CUDA / HIP. It does not scale with
  problem size, so it is negligible for large GPU SpMV and a real
  fraction only for many small `apply()`s (batched/block solvers,
  strong-scaling limits — the regimes the paper's Table 1 itself uses).
- This does **not** propose changing Ginkgo's `core/`. `LinOp`/`Executor`
  do real work (heterogeneous containers, runtime format choice, logging
  hooks) that a compile-time approach would break. The narrow claim is:
  *if a format's backend set is known at compile time, forks 2+3 are
  avoidable while staying a `LinOp`.*
- `ServesExec` is not new HAPI machinery — `hapi::FindFirst<Q>` + the
  predicate protocol are stock `meta.h`; `hapi::FindFirst<hapi::TagIs<…>>`
  covers "select a chain member by tag" directly.
