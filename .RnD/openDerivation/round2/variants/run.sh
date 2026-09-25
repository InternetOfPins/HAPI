#!/bin/bash
# Round 2 variants: (a) hapi::APIOf written in ':' syntax has the same base as the real one;
# (b) Cell written as a bare pack fold instead of hapi::APIOf, run through check/build.sh unchanged: what breaks.
cd "$(dirname "$0")"
R=$(cd ../../../.. && pwd); SN=$R/examples/static_net; H=${HAPI:-$R/include}
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
echo "== (a) APIOf in ':' syntax"
python3 ../../translate.py --report apiof/apiof_od.cpp -o apiof/apiof_od.out.cpp
for c in g++ clang++; do $c -std=c++17 -I"$H" apiof/apiof_od.out.cpp -o "$W/a" && echo "  ok    apiof [$c]: static_asserts hold"; done
echo "== (b) Cell = (OO : ... : Bias<k> : API), CellOf likewise"
python3 ../../translate.py --report --outdir cell_fold/out cell_fold/waveCell.h cell_fold/linCell.h
d=$W/static_net; mkdir -p "$d"; cp -r "$SN/check" "$SN/include" "$d/"; ln -s "$SN/models" "$d/models"
cp cell_fold/out/*.h "$d/include/"
HAPI="$H" "$d/check/build.sh" > cell_fold/build.txt 2>&1
grep -E "FAIL|ok, " cell_fold/build.txt
echo "== (b') codegen: bare fold vs hapi::APIOf, check/ AVR programs and the compare_emlearn rows"
o=$W/orig; mkdir -p "$o"; cp -r "$SN/check" "$SN/include" "$SN/compare_emlearn" "$SN/measure" "$o/"; ln -s "$SN/models" "$o/models"
cp -r "$SN/compare_emlearn" "$SN/measure" "$d/"; rm -rf "$o/compare_emlearn/out" "$d/compare_emlearn/out"
n=0; same=0
for s in mixed_avr_size refid_avr sugar_avr sugar_hand_avr sonar_lin_avr_size sonar_lin_avr_size_wide sugar_roll_avr sugar_roll_hand_avr banknote_avr_check; do
  for m in "$o" "$d"; do ( cd "$m/check" && avr-g++ -std=c++17 -Os -mmcu=atmega328p -I"$H" -I. -I../include -I../models/banknote -I../models/sonar -I../models/roll60 $s.cpp -o x.elf && avr-objdump -d x.elf | grep -v 'file format' > $s.dis ); done
  n=$((n+1)); cmp -s "$o/check/$s.dis" "$d/check/$s.dis" && same=$((same+1)) || echo "  FAIL  $s: disassembly differs"
done
echo "  $same/$n identical disassembly (bare fold vs APIOf)"
for p in "APIOf:$o" "fold:$d"; do lbl=${p%%:*}; m=${p#*:}
  ( cd "$m/compare_emlearn" && HAPI="$H" ./run.sh null wave4 lin4 >/dev/null 2>&1; grep -E '^\| (wave4|lin4) ' results.md | sed "s/^/  $lbl: /" ); done
echo "== (c) the same bare-fold Cell, --lower=nested (od::FoldT: no Chain wrapper, so no Types), through check/build.sh"
python3 ../../translate.py --lower=nested --outdir cell_fold/out_nested cell_fold/waveCell.h cell_fold/linCell.h
for h in waveCell linCell; do sed -i 's|#include "staticNet.h"|#include "staticNet.h"\n#include "od_fold.h"|' cell_fold/out_nested/$h.h; done
n=$W/nested/static_net; mkdir -p "$n"; cp -r "$SN/check" "$SN/include" "$n/"; ln -s "$SN/models" "$n/models"
cp cell_fold/out_nested/*.h ../../support/od_fold.h "$n/include/"
HAPI="$H" "$n/check/build.sh" > cell_fold/build_nested.txt 2>&1
grep -E "FAIL|ok, " cell_fold/build_nested.txt
