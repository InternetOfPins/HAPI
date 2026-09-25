#!/usr/bin/env bash
# single/hapi.h (scripts/amalgamate.py) against include/hapi/*.h: the same programs, built both ways, must be the same programs.
#   1. single/hapi.h is current (amalgamate.py --check)
#   2. host: examples/{rules,std,crtp,free,virt}, g++ and clang++ (-O2): identical `objdump -d`, identical output
#   3. AVR (when avr-g++ is present): static_net programs at -Os for the ATmega328p: identical `avr-objdump -d`
# The single-header build sees ONLY hapi/hapi.h (a copy of single/hapi.h in a scratch include dir), so a program that
# still reaches for another include/hapi/*.h fails to compile instead of silently using include/.
# Usage: tests/single_header/run.sh   (exit status 1 on any mismatch; not part of CI's tests/*.cpp glob)
set -u
cd "$(dirname "$0")/../.."
R=$PWD
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
mkdir -p "$W/single/hapi" && cp single/hapi.h "$W/single/hapi/hapi.h"
pass=0; fail=0
ok()  { echo "  ok    $1"; pass=$((pass+1)); }
bad() { echo "  FAIL  $1: $2"; fail=$((fail+1)); }
have() { command -v "$1" >/dev/null 2>&1; }

echo "== single/hapi.h is current"
python3 scripts/amalgamate.py --check >/dev/null 2>"$W/err" && ok "single/hapi.h matches scripts/amalgamate.py" || bad "single/hapi.h" "$(cat "$W/err")"

# both NAME DIS_TOOL COMPILE...   COMPILE is run twice, with HAPI_INC set to include/ and to the single-header dir
both() { local n=$1 dis=$2; shift 2
  for m in include single; do
    [ $m = include ] && HAPI_INC=$R/include || HAPI_INC=$W/single
    ( eval "$@" -o "$W/$m.elf" ) 2>"$W/err" || { bad "$n [$m]" "does not compile: $(grep -m1 error "$W/err")"; return 1; }
    $dis -d "$W/$m.elf" | grep -v 'file format' > "$W/$m.dis"
  done
  if cmp -s "$W/include.dis" "$W/single.dis"; then ok "$n: identical disassembly ($(wc -l < "$W/single.dis") lines)"; return 0
  else bad "$n" "disassembly differs"; return 1; fi; }

echo "== host examples"
for cxx in g++ clang++; do have $cxx || continue
  for ex in rules std crtp free virt; do
    if both "$ex [$cxx]" objdump $cxx -std=c++17 -O2 -I\"\$HAPI_INC\" examples/$ex/src/main.cpp; then
      [ "$("$W/include.elf" 2>&1)" = "$("$W/single.elf" 2>&1)" ] && ok "$ex [$cxx]: same output" || bad "$ex [$cxx]" "outputs differ"
    fi
  done
done

if have avr-g++ && have avr-objdump; then
  echo "== AVR (static_net, avr-g++ -Os -mmcu=atmega328p)"
  SN=examples/static_net
  SNI="-I$SN/include -I$SN/models/banknote -I$SN/models/sonar -I$SN/models/roll60"
  for p in mixed_avr_size refid_avr sugar_avr sugar_hand_avr sonar_lin_avr_size sugar_roll_avr banknote_avr_check "roll_avr -DUSE=Rolled" "roll_avr -DUSE=Unrolled"; do
    set -- $p; src=$1; shift
    both "$src $* [avr]" avr-objdump avr-g++ -std=c++17 -Os -mmcu=atmega328p $* -I\"\$HAPI_INC\" -I$SN/check $SNI $SN/check/$src.cpp
  done
  for m in wave4:bnc_wave.h lin4:bnc_lin.h; do n=${m%%:*}; h=${m#*:}
    both "bnc $n [avr]" avr-objdump avr-g++ -std=c++17 -Os -mmcu=atmega328p -ffunction-sections -fdata-sections -Wl,--gc-sections \
      -I\"\$HAPI_INC\" -I$SN/compare_emlearn -I$SN/measure -I$SN/compare_emlearn/emlearn $SNI \
      "-DBNC_NAME='\"$n\"'" "-DBNC_MODEL='\"$h\"'" $SN/compare_emlearn/bnc.cpp -lm
  done
else
  echo "  note  avr-g++ not found: AVR comparison skipped"
fi

echo; echo "$pass ok, $fail FAIL"; [ $fail -eq 0 ]
