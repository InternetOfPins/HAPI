#!/usr/bin/env bash
# Full reproducer with plain g++ (no PlatformIO): build every variant,
# disassemble apply_impl and show the fork structure, run correctness +
# timing. Needs a reference-only Ginkgo -- run ./setup_ginkgo.sh first,
# or point GINKGO_DIR at an existing checkout.
set -euo pipefail
cd "$(dirname "$0")"

GK=${GINKGO_DIR:-$HOME/ginkgo-src}
GKB=$GK/build
[ -d "$GKB/lib" ] || { echo "no Ginkgo build at $GKB -- run ./setup_ginkgo.sh"; exit 1; }

INC="-I$GK/include -I$GKB/include -I../../include"
LIB="-L$GKB/lib -Wl,-rpath,$GKB/lib -lginkgo -lginkgo_reference -lginkgo_omp \
     -lginkgo_cuda -lginkgo_hip -lginkgo_dpcpp -lginkgo_device"
STD="-std=c++17 -O2"
O=$(mktemp -d)
trap 'rm -rf "$O"' EXIT

build() { g++ $STD $INC ${3:-} src/$1.cpp -o "$O/$2" $LIB; }
obj()   { g++ $STD $INC ${3:-} -c src/$1.cpp -o "$O/$2.o"; }

calls() {  # $1=object  $2=demangled-symbol-substring
  objdump -dCr "$O/$1.o" | \
    awk -v s="$2" 'index($0,s) && /apply_impl\(gko::LinOp const\*, gko::LinOp\*\) const>:$/{f=1}
                   f&&/^$/{exit} f' | \
    grep -E '\tcall|R_X86_64_(PLT32|PC32)' || true
}

echo "### stencil -- custom-matrix-format"
build stencil sten_base
build stencil sten_fixed -DFIXED_EXEC
obj   stencil sten_base
obj   stencil sten_fixed -DFIXED_EXEC
echo "--- baseline :: apply_impl(const LinOp*, LinOp*) -- calls only ---"
calls sten_base  "StencilMatrix<double>"
echo "--- fixed-exec :: apply_impl(const LinOp*, LinOp*) -- calls only ---"
calls sten_fixed "StencilMatrixCT<double, gko::ReferenceExecutor"

echo; echo "### advdiff -- advection-diffusion vs gko::Combination"
build advdiff ad_comb
build advdiff ad_pre  -DPREFOLD
build advdiff ad_hapi -DHAPI
obj   advdiff ad_hapi -DHAPI
echo "--- ad_hapi :: apply_impl(const LinOp*, LinOp*) -- calls only ---"
calls ad_hapi "AdvDiffHapi<double>"

echo; echo "### run"
PIN=${PIN:-taskset -c 2}
for p in sten_base sten_fixed ad_comb ad_pre ad_hapi; do $PIN "$O/$p"; done
