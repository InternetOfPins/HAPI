#!/bin/bash
# Round 2 variants:
#   (a) hapi::APIOf written in ':' syntax (a struct over the fold) has the same base and Base as the real one; what it lacks
#   (b) EXPERIMENTAL --lower=nested: the round-2 headers (struct Cell over od::FoldT, no Chain wrapper) through check/build.sh
cd "$(dirname "$0")"
R=$(cd ../../../.. && pwd); SN=$R/examples/static_net; H=${HAPI:-$R/include}
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
echo "== (a) APIOf in ':' syntax"
python3 ../../translate.py --report apiof/apiof_od.cpp -o apiof/apiof_od.out.cpp
for c in g++ clang++; do $c -std=c++17 -I"$H" apiof/apiof_od.out.cpp -o "$W/a" && echo "  ok    apiof [$c]: static_asserts hold"; done
echo "== (b) EXPERIMENTAL: --lower=nested (od::FoldT: no Chain wrapper, so no Types) through check/build.sh"
python3 ../../translate.py --lower=nested --report --outdir nested/out ../src/waveCell.h ../src/linCell.h
for h in waveCell linCell; do sed -i 's|#include "staticNet.h"|#include "staticNet.h"\n#include "od_fold.h"|' nested/out/$h.h; done
n=$W/nested/static_net; mkdir -p "$n"; cp -r "$SN/check" "$SN/include" "$n/"; ln -s "$SN/models" "$n/models"
cp nested/out/*.h ../../support/od_fold.h "$n/include/"
HAPI="$H" "$n/check/build.sh" > nested/build.txt 2>&1
grep -E "FAIL|ok, " nested/build.txt
