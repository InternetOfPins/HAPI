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
echo "ALL PASS"
