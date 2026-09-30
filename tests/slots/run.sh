#!/usr/bin/env bash
# hapi/slots.h costs nothing on AVR: a typed slot access is the same flashed program as the hand-indexed array (avr-g++ -Os, ATmega328p).
# Compares the flashed bytes (.text + .data) of tests/slots/core_avr.cpp with and without -DFLAT. Skipped without avr-g++.
# Usage: tests/slots/run.sh   (exit status 1 on a difference; not part of CI's tests/*.cpp glob)
set -u
cd "$(dirname "$0")/../.."
if ! command -v avr-g++ >/dev/null 2>&1 || ! command -v avr-objcopy >/dev/null 2>&1; then echo "  note  avr-g++ not found: skipped"; exit 0; fi
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
for v in typed flat; do
  fl=""; [ $v = flat ] && fl="-DFLAT"
  avr-g++ -std=c++17 -Os -mmcu=atmega328p $fl -Iinclude tests/slots/core_avr.cpp -o "$W/$v.elf" || { echo "  FAIL  $v does not compile"; exit 1; }
  avr-objcopy -O binary -j .text -j .data "$W/$v.elf" "$W/$v.bin"
done
if cmp -s "$W/typed.bin" "$W/flat.bin"; then echo "  ok    typed slots == hand-indexed array: identical flashed image ($(stat -c %s "$W/typed.bin") bytes)"; else echo "  FAIL  the flashed images differ"; exit 1; fi
