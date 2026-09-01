#!/usr/bin/env bash
# Plain-g++ reproducer for the rosCompose example — no PlatformIO needed.
# Builds both sources native + avr-g++ and reports the composition facts.
set -e
cd "$(dirname "$0")"
INC="-I include -I ../../include"
CHIP="-I ../../../OneChip/include"
OUT=$(mktemp -d); trap 'rm -rf "$OUT"' EXIT

echo "=== node_avr (topic + service + action, one composed Node) ==="
g++ -std=c++17 -O2 -Wall -Wextra $INC src/node_avr.cpp -o "$OUT/node"
echo "  native run exit = $("$OUT/node"; echo $?)"
avr-g++ -std=c++17 -Os -mmcu=atmega328p -DF_CPU=16000000UL $INC -c src/node_avr.cpp -o "$OUT/node.o"
avr-size "$OUT/node.o"
echo "  icall in the object: $(avr-objdump -dCr "$OUT/node.o" | grep -c '\bicall\b')"
echo "    — each is a Cap::deliver transport hop or a user-supplied callback"
echo "      (service handler, response continuation). None is composition"
echo "      machinery: the fan-out fold, slot lookup, goal table and"
echo "      transition() are all straight-line."
echo "  malloc / soft-division: \
$(avr-objdump -dCr "$OUT/node.o" | grep -cE 'malloc|__divmod|__udivmod')  (want 0)"

echo
echo "=== qos_decorators (WithHistory + WithDeadline over the same subscriber) ==="
g++ -std=c++17 -O2 -Wall -Wextra $INC $CHIP src/qos_decorators.cpp -o "$OUT/qos"
echo "  native run exit = $("$OUT/qos"; echo $?)   (12 msgs + 8 retained + 0 missed = 20)"
avr-g++ -std=c++17 -Os -mmcu=atmega328p -DIOP -DF_CPU=16000000UL $INC $CHIP -c src/qos_decorators.cpp -o "$OUT/qos.o"
avr-size "$OUT/qos.o"
echo "  malloc / soft-division: \
$(avr-objdump -dCr "$OUT/qos.o" | grep -cE 'malloc|__divmod|__udivmod')  (want 0 — ring wraps with a conditional subtract, not % N)"
