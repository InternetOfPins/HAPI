# cutlass_layout

A HAPI `Chain<>` stack wrapping a real NVIDIA **CuTe** (`cute::Layout`,
part of the CUTLASS project) for coordinate math, with two independent
HAPI-composed concerns layered on top: bounds-checking and OneData-backed
access counting. Proves HAPI composes a **spatial** indexing library as
cleanly as it already composes temporal DSP pipelines (OneHLS) or parse
pipelines (OneParse/`config_loader`) — the exact gap the earlier
`OneHLS × CuTe` design-discipline study flagged and never closed (that
study only ever read CuTe's *documentation*; nothing in it ever compiled
real CuTe code).

No GPU, no `nvcc`, no CUDA toolkit is required to build or run this — see
"What's deliberately not attempted" below for why that's a real, checked
boundary, not an oversight.

## What it does

Two real `cute::Layout` instances share the same underlying 4×4 buffer:
one plain row-major, one a genuine nested/hierarchical CuTe shape
(`((2,2),(2,2))`, matching strides) — CuTe's own tiling capability, used
for real. The exact same `BoundsCheck`/`AccessLog` HAPI code wraps both
layouts unmodified; swapping which layout `LayoutIndex<>` is instantiated
with is the only change needed to switch indexing schemes.

## Run

```sh
export CUTLASS_INCLUDE=/path/to/cutlass/include
export CUDA_STUB_INCLUDE=/path/to/cuda_min_headers/include
pio run -e native
.pio/build/native/program
```

## One-time setup

`CUTLASS_INCLUDE`: a shallow clone of the real repo.

```sh
git clone --depth 1 https://github.com/NVIDIA/cutlass
```

`CUDA_STUB_INCLUDE`: `cute/config.hpp` transitively `#include`s
`<cuda_runtime_api.h>` **unconditionally** — not gated by `__CUDACC__` —
even though the layout algebra itself never touches a real CUDA runtime
(`CUTE_HOST_DEVICE` degrades to plain `inline` when `__CUDACC__` isn't
defined). Plain g++ still needs the *headers* present to parse that
`#include`, though nothing in this example ever links against or calls a
real CUDA function. Three small, official NVIDIA header-only PyPI
packages (~22MB total) satisfy the full transitive chain — no toolkit, no
driver, no `nvcc`:

```sh
pip download --no-deps -d /tmp/cuda_hdrs \
    nvidia-cuda-runtime-cu12 nvidia-cuda-nvcc-cu12 nvidia-cuda-cccl-cu12
cd /tmp/cuda_hdrs && for w in *.whl; do python3 -m zipfile -e "$w" extracted/; done
mkdir -p ~/cuda_min_headers/include
cp -r extracted/nvidia/cuda_runtime/include/. ~/cuda_min_headers/include/
cp -r extracted/nvidia/cuda_nvcc/include/.    ~/cuda_min_headers/include/
cp -r extracted/nvidia/cuda_cccl/include/.    ~/cuda_min_headers/include/
export CUDA_STUB_INCLUDE=~/cuda_min_headers/include
```

Each package supplies one real, distinct link in the chain, found by
actually compiling and fixing errors one at a time, not guessed in
advance: `nvidia-cuda-runtime-cu12` for `cuda_runtime_api.h` itself;
`nvidia-cuda-nvcc-cu12` for `crt/host_defines.h` (`cuda_runtime_api.h`'s
own next include); `nvidia-cuda-cccl-cu12` for `cuda/std/*` (libcu++,
pulled in by `cutlass/cutlass.h`). These packages carry NVIDIA's own
license (not BSD-3-Clause like CUTLASS itself, and not this repository's
license) — see each package's bundled `License.txt`.

## What's deliberately not attempted, and why

- **`cutlass::reference::host::Gemm`** (CUTLASS's own CPU-only reference
  GEMM, used for correctness-checking) — despite its file comment saying
  "host-side code," it transitively includes
  `cutlass/numeric_conversion.h`, which references raw PTX builtins
  (`__dp4a`, `__hfma2`, `__byte_perm`) as ordinary (non-dependent) names
  inside class templates. That's a hard two-phase-lookup parse error
  without the real `nvcc` *frontend* — not a missing header, not a
  missing GPU, `nvcc` itself. A real, checked asymmetry: CuTe's layout
  algebra is genuinely host-portable; CUTLASS's own machinery is not,
  whatever its own docs claim about "host-side code."
- **Real device kernels** — this machine's GPU (GeForce GT 710, Kepler,
  compute capability 3.5) predates CUTLASS's own stated minimum by a full
  architecture generation in every version checked: current `main`'s
  README states minimum Volta (compute capability 7.0); the real
  historical `v1.0.1` tag's README lists Maxwell/Pascal/Volta as the
  supported range and never mentions Kepler. Not a toolchain-install gap
  closeable later — the hardware itself is the dead end.

## Two real bugs caught while building this

1. The exact `using-namespace collision` class already known elsewhere in
   this project: a blanket `using namespace oneData;` collides
   `oneData::Int` (`= Data<int>`) against `cute::Int<v>` the moment both
   are visible unqualified in the same scope. Fixed by never
   blanket-importing `oneData` — every `oneData::` reference in
   `src/main.cpp` stays qualified.
2. `RawStore<N>` is a real HAPI *component* (has its own nested
   `Part<O>`), so it must live in `APIOf`'s `OO...` list — passing it as
   `APIOf`'s terminal-API slot instead (which wants a plain, `Part`-less
   fallback, matching `config_loader`'s `ConfigAPI`) silently compiles
   into the wrong shape and fails with a confusing "no member named
   `data`" error much later at the call site.
