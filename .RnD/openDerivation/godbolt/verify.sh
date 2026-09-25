#!/bin/bash
# Build both Compiler Explorer sources the way Compiler Explorer would (avr-g++ -std=c++17 -Os -mmcu=atmega328p) against the
# exact single/hapi.h their #include URL serves, and check:
#   1. wave4_od.cpp and wave4_apiof.cpp give the same wave4() (disassembly)
#   2. that wave4() is the one static_net builds as bnc_predict for compare_emlearn (examples/static_net, include/ headers)
# Needs avr-g++, avr-objdump, avr-nm; fetches the URL with curl, or uses ../../../single/hapi.h if it is byte-identical to the pin.
cd "$(dirname "$0")"
R=$(cd ../../.. && pwd); W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
pass=0; fail=0
ok()  { echo "  ok    $1"; pass=$((pass+1)); }
bad() { echo "  FAIL  $1: $2"; fail=$((fail+1)); }
URL=$(grep -o 'https://raw.githubusercontent.com/[^>]*' wave4_od.cpp | head -1)
SHA=$(echo "$URL" | sed -E 's|.*/HAPI/([0-9a-f]+)/.*|\1|')
if curl -sS -m 30 -o "$W/hapi.h" "$URL" 2>/dev/null && [ -s "$W/hapi.h" ]; then ok "fetched $URL"
elif git -C "$R" show "$SHA:single/hapi.h" > "$W/hapi.h" 2>/dev/null; then ok "no network: single/hapi.h from commit $SHA (what the URL serves)"
else bad "single/hapi.h" "cannot fetch $URL"; echo "$pass ok, $fail FAIL"; exit 1; fi
FLAGS="-std=c++17 -Os -mmcu=atmega328p"
dis() { avr-objdump -d "$1" | awk -v f="<$2>:" '$2==f{p=1;next} p&&/^$/{exit} p' | cut -f2- ; }   # one function's instructions
for s in od apiof; do
  sed "s|#include <$URL>|#include \"$W/hapi.h\"|" wave4_$s.cpp > "$W/$s.cpp"
  avr-g++ $FLAGS -c "$W/$s.cpp" -o "$W/$s.o" 2>"$W/err" && ok "wave4_$s.cpp compiles (avr-g++ $(avr-g++ -dumpversion) $FLAGS)" || bad "wave4_$s.cpp" "$(grep -m1 error "$W/err")"
  sym=$(avr-nm "$W/$s.o" | awk '$3 ~ /wave4/{print $3}'); dis "$W/$s.o" "$sym" > "$W/$s.dis"
  hex=$(avr-nm -S "$W/$s.o" | awk '$4 ~ /wave4/{print $2}'); echo "        wave4(): $((16#$hex)) B of code"
done
cmp -s "$W/od.dis" "$W/apiof.dis" && ok "wave4_od.cpp and wave4_apiof.cpp: identical wave4() ($(wc -l < "$W/od.dis") instructions)" || { bad "wave4()" "the two sources differ"; diff "$W/od.dis" "$W/apiof.dis" | head; }
SN=$R/examples/static_net
( cd "$SN/compare_emlearn" && avr-g++ $FLAGS -I"$R/include" -I. -I../include -I../models/banknote -I../measure -Iemlearn \
    -DBNC_NAME='"wave4"' -DBNC_MODEL='"bnc_wave.h"' -c bnc.cpp -o "$W/bnc.o" ) 2>"$W/err" || bad "bnc.cpp" "$(grep -m1 error "$W/err")"
dis "$W/bnc.o" "$(avr-nm "$W/bnc.o" | awk '$3 ~ /bnc_predict/{print $3}')" > "$W/bnc.dis"
cmp -s "$W/od.dis" "$W/bnc.dis" && ok "wave4() == static_net's bnc_predict (compare_emlearn, include/ headers)" || { bad "vs bnc_predict" "differs"; diff "$W/od.dis" "$W/bnc.dis" | head; }
echo; echo "$pass ok, $fail FAIL"; [ $fail -eq 0 ]
