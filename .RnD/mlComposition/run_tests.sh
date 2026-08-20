#!/usr/bin/env bash
# HAPI + ML composition experiment -- see HANDOFF.md
# Usage: ./run_tests.sh /path/to/IOP/root
set -euo pipefail

IOP_ROOT="${1:?path to IOP root (parent of HAPI/OneData/OneHLS/OneChip/OneIO) required}"
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

HAPI_INC="$IOP_ROOT/HAPI/include"
AVR_GPP="$HOME/.platformio/packages/toolchain-atmelavr/bin/avr-g++"
AVR_OBJDUMP="$HOME/.platformio/packages/toolchain-atmelavr/bin/avr-objdump"
AVR_OBJCOPY="$HOME/.platformio/packages/toolchain-atmelavr/bin/avr-objcopy"

pass() { echo "PASS: $1"; }
fail() { echo "FAIL: $1"; exit 1; }

echo "== Round 2: A/B/C native correctness vs boring baseline =="
g++ -std=c++17 -O2 -I "$HAPI_INC" "$HERE/variants.cpp" -o "$WORK/variants"
"$WORK/variants" | tee "$WORK/variants.log"
grep -q "ALL VARIANTS MATCH BORING BASELINE" "$WORK/variants.log" \
  && pass "A/B/C composed output matches boring baseline on all 5 test inputs" \
  || fail "variants.cpp reported a mismatch"

echo "== Round 2: required-but-missing compile failure =="
if g++ -std=c++17 -I "$HAPI_INC" -c "$HERE/missing_api.cpp" -o "$WORK/missing.o" 2>"$WORK/missing.log"; then
  fail "missing_api.cpp compiled -- should fail"
fi
grep -q "tempC10.*is not a member of.*EmptyTerminal" "$WORK/missing.log" \
  && pass "missing_api: expected compile error" \
  || fail "missing_api: unexpected error, got:\n$(cat "$WORK/missing.log")"

echo "== Round 2: A/B/C AVR footprint, 0 indirect calls =="
for v in 0 1 2; do
  name=$([ "$v" = 0 ] && echo A || ([ "$v" = 1 ] && echo B || echo C))
  "$AVR_GPP" -std=c++17 -Os -mmcu=atmega328p -DVARIANT=$v -I "$HAPI_INC" \
    "$HERE/variants_avr.cpp" -o "$WORK/variant_$name.elf"
  ic=$("$AVR_OBJDUMP" -d "$WORK/variant_$name.elf" | grep -c 'icall\|callx' || true)
  [ "$ic" = 0 ] && pass "variant $name: 0 indirect calls" || fail "variant $name: $ic indirect calls found"
done

echo "== Round 1: stage2 PoC, dropout elimination =="
"$AVR_GPP" -std=c++17 -Os -mmcu=atmega328p -I "$HAPI_INC" \
  "$HERE/stage2_inference_poc.cpp" -o "$WORK/net.elf"
"$AVR_GPP" -std=c++17 -Os -mmcu=atmega328p -DWITH_DROPOUT -I "$HAPI_INC" \
  "$HERE/stage2_inference_poc.cpp" -o "$WORK/net_drop.elf"
"$AVR_OBJCOPY" -O binary -j .text "$WORK/net.elf"      "$WORK/text_nodrop.bin"
"$AVR_OBJCOPY" -O binary -j .text "$WORK/net_drop.elf" "$WORK/text_drop.bin"
cmp -s "$WORK/text_nodrop.bin" "$WORK/text_drop.bin" \
  && pass "stage2: .text byte-identical with/without DropoutNoop" \
  || fail "stage2: .text differs with DropoutNoop spliced in"
ic=$("$AVR_OBJDUMP" -d "$WORK/net.elf" | grep -c 'icall\|callx' || true)
[ "$ic" = 0 ] && pass "stage2: 0 indirect calls" || fail "stage2: $ic indirect calls found"

echo "== Round 1: stage1 pipeline, needs a real PlatformIO env =="
echo "  (not automated here -- requires OneData/OneHLS/OneChip/OneIO symlink:// lib_deps"
echo "   and a PlatformIO uno build; see HANDOFF.md for the numbers last verified)"

echo
echo "ALL PASS"
