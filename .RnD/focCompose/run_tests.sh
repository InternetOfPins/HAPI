#!/usr/bin/env bash
# focCompose experiment -- see HANDOFF.md
# Usage: ./run_tests.sh /path/to/IOP/root
set -euo pipefail

IOP_ROOT="${1:?path to IOP root (parent of HAPI) required}"
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

export PATH="$HOME/.platformio/penv/bin:$PATH"
AVR_OBJDUMP="$HOME/.platformio/packages/toolchain-atmelavr/bin/avr-objdump"

pass() { echo "PASS: $1"; }
fail() { echo "FAIL: $1"; exit 1; }

echo "== focCompose: Chain<Encoder,BLDCDriver3PWM>::Part<MotorTerminal> on real AVR =="
mkdir -p "$WORK/pio/src"
cp "$HERE/focCompose.cpp" "$WORK/pio/src/main.cpp"
cp "$HERE/focAPI.h" "$WORK/pio/src/"
cp "$HERE/stub_foc.h" "$WORK/pio/src/"
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
	HAPI=symlink://$IOP_ROOT/HAPI
EOF

if pio run -d "$WORK/pio" -e uno > "$WORK/build.log" 2>&1; then
  pass "focCompose.cpp builds clean via PlatformIO uno (static_asserts in the file itself pin !is_polymorphic, sizeof==1, and the init() name-hiding finding)"
else
  cat "$WORK/build.log"
  fail "PlatformIO build failed -- see log above"
fi

grep -E "^(RAM|Flash):" "$WORK/build.log"

ic=$("$AVR_OBJDUMP" -d "$WORK/pio/.pio/build/uno/firmware.elf" | grep -c 'icall\|callx' || true)
[ "$ic" = 0 ] && pass "0 indirect calls in the full linked firmware" || fail "$ic indirect calls found"

echo
echo "ALL PASS"
