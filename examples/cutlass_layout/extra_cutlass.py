"""
Wires CUTLASS_INCLUDE and CUDA_STUB_INCLUDE into the native build.

This example uses real NVIDIA cute::Layout (from CUTLASS) for coordinate
math -- no GPU or nvcc is needed to build or run it, but cute/config.hpp
transitively pulls <cuda_runtime_api.h> unconditionally (not gated by
__CUDACC__), so plain g++ needs SOME real CUDA headers on the include path
even though it never calls into a real CUDA runtime. See this example's
README for how that dependency chain was actually traced (and why
cutlass::reference::host::Gemm is a real dead end here, unlike Layout).

Requires:
  CUTLASS_INCLUDE  - path to a clone's include/ dir
                     (git clone --depth 1 https://github.com/NVIDIA/cutlass)
  CUDA_STUB_INCLUDE - path to a merged directory containing just enough of
                     NVIDIA's own header-only PyPI packages to satisfy
                     cute/config.hpp's include chain -- no toolkit, no
                     driver, no nvcc. See README.md for the exact
                     `pip download` + merge steps.

Same env-var-gated shape as OneHLS's AC_TYPES_INCLUDE
(examples/hls_float_fir/extra_hls.py) -- omitting either var fails the
build with a clear message, it doesn't silently fall back to anything.
"""
import os
Import("env")

CUTLASS_INC = os.environ.get("CUTLASS_INCLUDE")
CUDA_STUB_INC = os.environ.get("CUDA_STUB_INCLUDE")

if not CUTLASS_INC or not CUDA_STUB_INC:
    missing = []
    if not CUTLASS_INC:
        missing.append("CUTLASS_INCLUDE (clone of https://github.com/NVIDIA/cutlass, "
                        "point at its include/ dir)")
    if not CUDA_STUB_INC:
        missing.append("CUDA_STUB_INCLUDE (merged NVIDIA header-only pip packages -- "
                        "see README.md)")
    print("cutlass_layout: missing required env var(s):")
    for m in missing:
        print("  - " + m)
    env.Exit(1)

env.Append(CPPPATH=[CUTLASS_INC, CUDA_STUB_INC])
