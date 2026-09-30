#!/bin/bash
# Build both Compiler Explorer sources the way Compiler Explorer would (avr-g++ -std=c++17 -Os -mmcu=atmega328p) against the
# exact single/hapi.h their #include URL serves, and check:
#   1. wave4_od.cpp and wave4_apiof.cpp give the same wave4() (disassembly)
#   2. that wave4() is the one static_net builds as bnc_predict for compare_emlearn (examples/static_net, include/ headers)
#   3. wave4_c.c, the same cell in plain C, against it: the size of each, the instructions that differ, and (with a host g++)
#      the same answer as the HAPI cell on all 2^32 inputs
# Needs avr-g++, avr-gcc, avr-objdump, avr-nm (and g++ for the exhaustive check); fetches the URL with curl, or uses ../../../single/hapi.h if it is byte-identical to the pin.
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
# plain C: compiled as C++ exactly like the other two, and as C
avr-g++ $FLAGS -x c++ -c wave4_c.c -o "$W/c.o" 2>"$W/err" && ok "wave4_c.c compiles as C++ (avr-g++ $FLAGS)" || bad "wave4_c.c" "$(grep -m1 error "$W/err")"
avr-gcc -std=c99 -Os -mmcu=atmega328p -c wave4_c.c -o "$W/cc.o" 2>"$W/err" && ok "wave4_c.c compiles as C (avr-gcc -std=c99 -Os -mmcu=atmega328p)" || bad "wave4_c.c as C" "$(grep -m1 error "$W/err")"
dis "$W/c.o" "$(avr-nm "$W/c.o" | awk '$3 ~ /wave4/{print $3}')" > "$W/c.dis"
dis "$W/cc.o" wave4 > "$W/cc.dis"
size() { echo $((16#$(avr-nm -S "$1" | awk '$4 ~ /wave4/{print $2}'))); }
echo "        wave4(), HAPI: $(wc -l < "$W/od.dis") instructions, $(size "$W/od.o") B; plain C: $(wc -l < "$W/c.dis") instructions, $(size "$W/c.o") B (as C: $(wc -l < "$W/cc.dis"), $(size "$W/cc.o") B)"
cmp -s "$W/c.dis" "$W/cc.dis" && ok "wave4_c.c: the same wave4() as C and as C++" || bad "wave4_c.c" "C and C++ builds differ"
echo "        HAPI (<) vs plain C (>), instructions in either one only:"
diff <(cut -f2,3 "$W/od.dis" | sort) <(cut -f2,3 "$W/c.dis" | sort) | grep '^[<>]' | sed 's/^/          /'
if command -v g++ >/dev/null; then
  sed 's|^bool wave4(|bool wave4_hapi(|' "$W/apiof.cpp" > "$W/h.cpp"
  { echo '#include "h.cpp"'; echo '#include <stdio.h>'; echo '#define wave4 wave4_c'; echo '#include "'"$PWD"'/wave4_c.c"'
    echo '__attribute__((noinline)) bool H(const uint8_t* x) {return wave4_hapi(x);}'
    echo '__attribute__((noinline)) bool C(const uint8_t* x) {return wave4_c(x);}'
    echo 'int main() {unsigned long long d=0; uint32_t i=0; do {uint8_t x[4]={uint8_t(i),uint8_t(i>>8),uint8_t(i>>16),uint8_t(i>>24)}; d+=H(x)!=C(x);} while(++i); printf("%llu\n",d); return d!=0;}'
  } > "$W/ex.cpp"
  g++ -std=c++17 -O2 -fno-ipa-icf -fno-ipa-pure-const "$W/ex.cpp" -o "$W/ex" 2>"$W/err" && d=$("$W/ex") \
    && ok "wave4_c.c == HAPI cell on all 2^32 inputs (host g++, $(g++ -dumpversion))" || bad "exhaustive check" "${d:-$(grep -m1 error "$W/err")} inputs differ"
else echo "  skip  exhaustive check (no host g++)"; fi
echo; echo "$pass ok, $fail FAIL"; [ $fail -eq 0 ]
