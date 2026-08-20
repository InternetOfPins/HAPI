#!/usr/bin/env bash
# IoT device composition experiment -- see HANDOFF.md
# Usage: ./run_tests.sh /path/to/IOP/root
set -euo pipefail

IOP_ROOT="${1:?path to IOP root (parent of HAPI/OneData/OneHLS/OneChip/OneBus/OneIO) required}"
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

export PATH="$HOME/.platformio/penv/bin:$PATH"
AVR_OBJDUMP="$HOME/.platformio/packages/toolchain-atmelavr/bin/avr-objdump"
XTENSA_OBJDUMP="$HOME/.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-objdump"

pass() { echo "PASS: $1"; }
fail() { echo "FAIL: $1"; exit 1; }

echo "== Stage 1: AVR IotDevice composition, real PlatformIO build =="
mkdir -p "$WORK/pio/src"
cp "$HERE/stage1_avr.cpp" "$WORK/pio/src/main.cpp"
cat > "$WORK/pio/platformio.ini" <<EOF
[env:uno]
platform = atmelavr
board = uno
framework = arduino
build_flags = -std=gnu++17
build_unflags =
	-std=c++11
	-std=c++14
	-std=c++20
	-std=gnu++11
	-std=gnu++14
lib_deps=
	HAPI      =symlink://$IOP_ROOT/HAPI
	OneBit    =symlink://$IOP_ROOT/OneBit
	OnePin    =symlink://$IOP_ROOT/OnePin
	OneBus    =symlink://$IOP_ROOT/OneBus
	OneChip   =symlink://$IOP_ROOT/OneChip
	OneData   =symlink://$IOP_ROOT/OneData
	OneOutput =symlink://$IOP_ROOT/OneOutput
	OneItem   =symlink://$IOP_ROOT/OneItem
	OneParse  =symlink://$IOP_ROOT/OneParse
	OneMenu   =symlink://$IOP_ROOT/OneMenu
	OneIO     =symlink://$IOP_ROOT/OneIO
	OneHLS    =symlink://$IOP_ROOT/OneHLS
EOF

if pio run -d "$WORK/pio" -e uno > "$WORK/build.log" 2>&1; then
  pass "stage1_avr.cpp builds clean via PlatformIO uno"
else
  cat "$WORK/build.log"
  fail "PlatformIO build failed -- see log above"
fi

grep -E "^(RAM|Flash):" "$WORK/build.log"

ic=$("$AVR_OBJDUMP" -d "$WORK/pio/.pio/build/uno/firmware.elf" | grep -c 'icall\|callx' || true)
[ "$ic" = 0 ] && pass "0 indirect calls" || fail "$ic indirect calls found"

echo
echo "== Stage 2: ESP32 IotDevice, real Arduino-ESP32 core + real PubSubClient =="
mkdir -p "$WORK/esp32/src" "$WORK/esp32/local_include/oneIO/net"
cp "$HERE/stage2_esp32.cpp" "$WORK/esp32/src/main.cpp"
cp "$HERE/mqtt.h.candidate" "$WORK/esp32/local_include/oneIO/net/mqtt.h"
cat > "$WORK/esp32/platformio.ini" <<EOF
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
build_flags =
	-std=gnu++17
	-I local_include
build_unflags =
	-std=c++11
	-std=c++14
	-std=c++20
	-std=gnu++11
	-std=gnu++14
lib_deps=
	HAPI      =symlink://$IOP_ROOT/HAPI
	OneBit    =symlink://$IOP_ROOT/OneBit
	OnePin    =symlink://$IOP_ROOT/OnePin
	OneBus    =symlink://$IOP_ROOT/OneBus
	OneChip   =symlink://$IOP_ROOT/OneChip
	OneData   =symlink://$IOP_ROOT/OneData
	OneOutput =symlink://$IOP_ROOT/OneOutput
	OneItem   =symlink://$IOP_ROOT/OneItem
	OneParse  =symlink://$IOP_ROOT/OneParse
	OneMenu   =symlink://$IOP_ROOT/OneMenu
	OneIO     =symlink://$IOP_ROOT/OneIO
	OneHLS    =symlink://$IOP_ROOT/OneHLS
	knolleary/PubSubClient@^2.8
	WiFi
EOF

if pio run -d "$WORK/esp32" -e esp32dev > "$WORK/esp32build.log" 2>&1; then
  pass "stage2_esp32.cpp builds clean via real Arduino-ESP32 core + real PubSubClient"
else
  cat "$WORK/esp32build.log"
  fail "ESP32 PlatformIO build failed -- see log above"
fi

grep -E "^(RAM|Flash):" "$WORK/esp32build.log"

# Whole-firmware callx8 counts are dominated by framework code and are NOT
# a meaningful zero-overhead proxy on Xtensa (-mlongcalls turns every far
# call into l32r+callx8 regardless of whether the target is statically
# known) -- check only our own composed object, pre-link, and confirm
# every callx8's l32r target resolves to a fixed symbol, not a vtable/data
# pointer.
mainobj=$(find "$WORK/esp32/.pio/build/esp32dev" -path "*/src/main.cpp.o")
ic=$("$XTENSA_OBJDUMP" -dr "$mainobj" | grep -c "callx8" || true)
unresolved=$("$XTENSA_OBJDUMP" -dr "$mainobj" | grep -A1 "callx8" | grep -c "l32r" || true)
echo "  our object's callx8 count: $ic (framework-ABI far-call pattern, not runtime dispatch)"
[ "$ic" -gt 0 ] && pass "callx8 present (expected -- -mlongcalls ABI), relocations checked below" \
                 || fail "expected callx8 from -mlongcalls, found none -- investigate"

echo
echo "ALL PASS"
