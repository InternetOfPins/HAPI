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

echo "### Round 2 -- StencilMatrix (custom-matrix-format)"
build stencil sten_base
build stencil sten_hapi -DHAPI
obj   stencil sten_base
obj   stencil sten_hapi -DHAPI

for pair in "sten_base:StencilMatrix<double>" \
            "sten_hapi:StencilMatrixHapi<double, gko::ReferenceExecutor"; do
  o=${pair%%:*}; sym=${pair#*:}
  echo "--- $o :: apply_impl(const LinOp*, LinOp*) -- calls only ---"
  objdump -dCr "$O/$o.o" | \
    awk -v s="$sym" 'index($0,s) && /apply_impl\(gko::LinOp const\*, gko::LinOp\*\) const>:$/{f=1}
                     f&&/^$/{exit} f' | \
    grep -E '\tcall|R_X86_64_(PLT32|PC32)' || true
done

echo; echo "### Round 3 -- advection-diffusion vs gko::Combination"
build advdiff ad_comb
build advdiff ad_pre  -DPREFOLD
build advdiff ad_hapi -DHAPI
obj   advdiff ad_hapi -DHAPI
echo "--- ad_hapi :: apply_impl(const LinOp*, LinOp*) -- calls only ---"
objdump -dCr "$O/ad_hapi.o" | \
  awk '/AdvDiffHapi<double>::apply_impl\(gko::LinOp const\*, gko::LinOp\*\) const>:$/{f=1}
       f&&/^$/{exit} f' | grep -E '\tcall|R_X86_64_PLT32' || true

echo; echo "### run"
PIN=${PIN:-taskset -c 2}
for p in sten_base sten_hapi ad_comb ad_pre ad_hapi; do $PIN "$O/$p"; done
