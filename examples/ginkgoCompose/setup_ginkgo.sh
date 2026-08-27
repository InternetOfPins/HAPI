#!/usr/bin/env bash
# One-time: clone + build a reference-only Ginkgo for this example.
# Reference executor only -- no CUDA/HIP/SYCL/OMP/MPI/tests/benchmarks,
# ~430 MB, a few minutes at -j2. Re-runnable (skips clone/configure if
# present); incremental rebuilds are instant.
#
#   ./setup_ginkgo.sh                     -> ~/ginkgo-src
#   GINKGO_DIR=/big/disk ./setup_ginkgo.sh
#
# Then: pio run -e stencil_hapi   (or ./verify.sh for the g++ reproducer)
set -euo pipefail

GK=${GINKGO_DIR:-$HOME/ginkgo-src}
GKB=$GK/build

mkdir -p "$(dirname "$GK")"
[ -d "$GK/.git" ] || \
  git clone --depth 1 https://github.com/ginkgo-project/ginkgo.git "$GK"

[ -f "$GKB/CMakeCache.txt" ] || cmake -S "$GK" -B "$GKB" \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON \
  -DGINKGO_BUILD_REFERENCE=ON \
  -DGINKGO_BUILD_OMP=OFF -DGINKGO_BUILD_CUDA=OFF -DGINKGO_BUILD_HIP=OFF \
  -DGINKGO_BUILD_SYCL=OFF -DGINKGO_BUILD_MPI=OFF \
  -DGINKGO_BUILD_TESTS=OFF -DGINKGO_BUILD_EXAMPLES=OFF \
  -DGINKGO_BUILD_BENCHMARKS=OFF -DGINKGO_BUILD_DOC=OFF

nice cmake --build "$GKB" -j"${JOBS:-2}"
echo "done: $GK  (set GINKGO_DIR=$GK if not the default)"
