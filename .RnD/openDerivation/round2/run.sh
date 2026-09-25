#!/bin/bash
# Round 2: static_net's waveCell.h / linCell.h written in ':' syntax (src/), translated (out/include/), then
#   1. round trip: the parts translate back byte-identical; the only lines that differ are Cell / CellOf, now a struct over
#      the pack fold closed with `final API` (struct-only form: hapi::APIOf<API,...> as its base) instead of an alias of hapi::APIOf (log/cell.diff)
#   2. two mirror trees of examples/static_net, identical except include/{waveCell,linCell}.h: "orig" and "od"
#   3. check/build.sh, unchanged, in both mirrors (host g++/clang++, must-not-build, AVR sizes, identical-disassembly, simavr rows)
#   4. compare_emlearn/run.sh wave4 lin4 in both (simavr: flash, RAM, agreement, cycles); measure/ bn_wave cycles
#   5. avr-objdump -d of every AVR program, built the same way in both mirrors: compared raw, and with symbol names
#      stripped (mangled names now say wave::Cell<...> where they said hapi::APIOf<...>)
# Needs g++, clang++, avr-g++ 7.3, avr-objdump, simavr, python3. Logs go to log/.
cd "$(dirname "$0")"
R=$(cd ../../.. && pwd); SN=$R/examples/static_net; H=${HAPI:-$R/include}
mkdir -p log out/include
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
pass=0; fail=0
ok()  { echo "  ok    $1"; pass=$((pass+1)); }
bad() { echo "  FAIL  $1: $2"; fail=$((fail+1)); }

echo "== 1. translate and round-trip"
python3 ../translate.py --report --outdir out/include src/waveCell.h src/linCell.h 2>&1 | tee log/translate.txt
: > log/cell.diff
for h in waveCell linCell; do
  diff "$SN/include/$h.h" out/include/$h.h >> log/cell.diff
  other=$(diff "$SN/include/$h.h" out/include/$h.h | grep '^[<>]' | grep -vE '^< +template<[^>]*> using Cell(Of)?=hapi::APIOf<|^> +template<[^>]*> struct Cell(Of)? : hapi::APIOf<' )
  n=$(diff "$SN/include/$h.h" out/include/$h.h | grep -c '^[<>]')
  [ -z "$other" ] && [ "$n" -eq 2 ] && ok "$h.h: byte-identical except the Cell line (alias of hapi::APIOf -> struct closed by APIOf with final API)" \
    || bad "$h.h round trip" "unexpected differences: $other"
done
mirror() { local d=$W/$1/static_net; mkdir -p "$d"
  cp -r "$SN/check" "$SN/compare_emlearn" "$SN/measure" "$SN/include" "$d/"; ln -s "$SN/models" "$d/models"
  rm -rf "$d/compare_emlearn/out"; echo "$d"; }
O=$(mirror orig); T=$(mirror od); cp out/include/waveCell.h out/include/linCell.h "$T/include/"

echo "== 2/3. check/build.sh (unchanged) in both mirrors"
for m in orig od; do d=$W/$m/static_net
  HAPI="$H" "$d/check/build.sh" > log/build_$m.txt 2>&1; rc=$?
  s=$(tail -1 log/build_$m.txt)
  [ $rc -eq 0 ] && ok "build.sh [$m]: $s" || bad "build.sh [$m]" "$s (log/build_$m.txt)"
done
diff <(sed 1d log/build_orig.txt) <(sed 1d log/build_od.txt) >/dev/null && ok "build.sh output identical, orig vs od" || bad "build.sh output" "differs (diff log/build_orig.txt log/build_od.txt)"

echo "== 4. compare_emlearn (simavr) and measure/ cycles"
for m in orig od; do d=$W/$m/static_net
  HAPI="$H" "$d/compare_emlearn/run.sh" null wave4 lin4 > log/emlearn_$m.txt 2>&1
  cp "$d/compare_emlearn/out/measure.txt" log/emlearn_measure_$m.txt; cp "$d/compare_emlearn/results.md" log/emlearn_results_$m.md
  ( cd "$d/measure" && for f in bn_wave bn_lin; do
      avr-g++ -std=c++17 -Os -mmcu=atmega328p -I"$H" -I. -I../include -I../models/banknote $f.cpp -o $f.elf &&
      timeout 10 simavr -m atmega328p -f 16000000 $f.elf 2>&1 | grep -aoE '[a-z0-9]+:[0-9]+'; done ) > log/measure_$m.txt 2>&1
done
w=$(grep '^| wave4 |' log/emlearn_results_od.md)   # net of the null program, as report.py prints it (README's table)
[[ "$w" == "| wave4 | 44 | 0 | 274/274 | 25 / 25.0 / 25 |"* ]] && ok "wave4 [od, simavr, net of null]: 44 B flash, 0 B RAM, 274/274, 25 cycles  ($w)" \
  || bad "wave4 [od]" "got '$w', want 44 B / 0 B / 274/274 / 25 cycles"
cmp -s log/emlearn_measure_orig.txt log/emlearn_measure_od.txt && ok "compare_emlearn rows identical, orig vs od: $(tr '\n' ';' < log/emlearn_measure_od.txt)" || bad "compare_emlearn" "orig and od rows differ"
cmp -s log/measure_orig.txt log/measure_od.txt && ok "measure/ cycles identical, orig vs od: $(tr '\n' ' ' < log/measure_od.txt)" || bad "measure/" "orig and od differ"

echo "== 5. avr-objdump -d, orig vs od, every AVR program"
# name|dir|source|extra flags     (the builds of check/build.sh, compare_emlearn/run.sh and measure/run.sh)
PROGS="mixed_avr_size|check|mixed_avr_size.cpp|
refid_avr|check|refid_avr.cpp|
sugar_avr|check|sugar_avr.cpp|
sugar_hand_avr|check|sugar_hand_avr.cpp|
sonar_lin_avr_size|check|sonar_lin_avr_size.cpp|
sonar_lin_avr_size_wide|check|sonar_lin_avr_size_wide.cpp|
roll_avr_unrolled|check|roll_avr.cpp|-DUSE=Unrolled
roll_avr_rolled|check|roll_avr.cpp|-DUSE=Rolled
roll_avr_sparse|check|roll_avr.cpp|-DUSE=Sparse
sugar_roll_avr|check|sugar_roll_avr.cpp|
sugar_roll_hand_avr|check|sugar_roll_hand_avr.cpp|
sugar_roll_avr_off|check|sugar_roll_avr.cpp|-DSUGAR_ROLL_AT=1000
sugar_unroll_hand_avr|check|sugar_unroll_hand_avr.cpp|
banknote_avr_check|check|banknote_avr_check.cpp|
sonar_rows_avr|check|sonar_rows_avr.cpp|-DSIM_ONCE
bnc_wave4|compare_emlearn|bnc.cpp|-ffunction-sections -fdata-sections -Wl,--gc-sections -I../measure -Iemlearn -DBNC_NAME='\"wave4\"' -DBNC_MODEL='\"bnc_wave.h\"' 
bnc_lin4|compare_emlearn|bnc.cpp|-ffunction-sections -fdata-sections -Wl,--gc-sections -I../measure -Iemlearn -DBNC_NAME='\"lin4\"' -DBNC_MODEL='\"bnc_lin.h\"' 
bn_wave|measure|bn_wave.cpp|
bn_lin|measure|bn_lin.cpp|
bn_table|measure|bn_table.cpp|
u32|measure|u32.cpp|
u16|measure|u16.cpp|
t60|measure|t60.cpp|
sonar_lin|measure|sonar_lin.cpp|
ru|measure|ru.cpp|
rr|measure|rr.cpp|
rs|measure|rs.cpp|"
: > log/objdump.txt
while IFS='|' read -r n dir src fl; do
  for m in orig od; do
    ( cd "$W/$m/static_net/$dir" && eval avr-g++ -std=c++17 -Os -mmcu=atmega328p $fl -I"$H" -I. -I../include -I../models/banknote -I../models/sonar -I../models/roll60 $src -o "$W/$m.$n.elf" -lm ) 2>"$W/err" \
      || { bad "objdump $n [$m]" "build: $(grep -m1 error "$W/err")"; continue 2; }
    avr-objdump -d "$W/$m.$n.elf" | grep -v 'file format' > "$W/$m.$n.dis"
    sed -E 's/<[^>]*>//g' "$W/$m.$n.dis" > "$W/$m.$n.ins"
  done
  sz=$(avr-size -C --mcu=atmega328p "$W/od.$n.elf" | awk '/^Program:/{p=$2}/^Data:/{d=$2}END{print p" B / "d" B"}')
  if cmp -s "$W/orig.$n.dis" "$W/od.$n.dis"; then
    ok "objdump $n: identical ($(wc -l < "$W/od.$n.dis") lines, $sz)"; echo "$n raw-identical $sz" >> log/objdump.txt
  elif cmp -s "$W/orig.$n.ins" "$W/od.$n.ins"; then
    ok "objdump $n: identical instructions, symbol names differ ($sz)"; echo "$n instructions-identical (symbol names differ) $sz" >> log/objdump.txt
    diff "$W/orig.$n.dis" "$W/od.$n.dis" | grep '^[<>]' | head -4 >> log/objdump.txt
  else bad "objdump $n" "instructions differ"; echo "$n DIFFERENT" >> log/objdump.txt; diff "$W/orig.$n.ins" "$W/od.$n.ins" | head -20 >> log/objdump.txt; fi
done <<< "$PROGS"

echo "== 6. the struct Cell next to hapi::APIOf (cell_vs_apiof.cpp)"
for c in g++ clang++; do
  $c -std=c++17 -I"$H" -Iout/include -I"$SN/include" cell_vs_apiof.cpp -o "$W/cv" 2>"$W/err" \
    && ok "cell_vs_apiof [$c]: own type, derived from exactly the APIOf it replaces, same Types (API first)" || bad "cell_vs_apiof [$c]" "$(grep -m1 error "$W/err")"
done

echo; echo "$pass ok, $fail FAIL"; [ $fail -eq 0 ]
