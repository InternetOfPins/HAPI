#!/usr/bin/env bash
# Rebuilds zero_overhead_demo.cpp and verifies run_static disassembles to a
# straight-line sequence (no indirect call/jmp), while run_dynamic contains
# one — i.e. HAPI's static composition still fully collapses after any
# change to include/hapi/*.h. Rerun this any time chain.h/meta.h/hapi.h
# change; it costs a couple seconds and needs no network/godbolt/AVR toolchain.
set -euo pipefail
cd "$(dirname "$0")"

CXX=${CXX:-clang++}
STD_FLAGS="-std=c++23 -O2 -Wall -Wextra -Wpedantic"

RESULTS_DIR=results
mkdir -p "$RESULTS_DIR"
REPORT="$RESULTS_DIR/zero_overhead_check.txt"

BIN=$(mktemp)
ASM=$(mktemp)
trap 'rm -f "$BIN" "$ASM"' EXIT

$CXX $STD_FLAGS -I../include -S -o "$ASM" zero_overhead_demo.cpp
$CXX $STD_FLAGS -I../include -o "$BIN" zero_overhead_demo.cpp

extract() {
  # print the body of a top-level asm label up to the next top-level label
  awk -v label="^${1}:" '
    $0 ~ label {found=1; print; next}
    found && /^[A-Za-z_.][A-Za-z0-9_.]*:/ {exit}
    found {print}
  ' "$ASM"
}

STATIC_BODY=$(extract run_static)
DYNAMIC_BODY=$(extract run_dynamic)

{
  echo "=== zero_overhead_check $(date -u +%FT%TZ) ==="
  echo "compiler: $($CXX --version | head -1)"
  echo "flags: $STD_FLAGS"
  echo
  echo "--- run_static ---"
  echo "$STATIC_BODY"
  echo
  echo "--- run_dynamic ---"
  echo "$DYNAMIC_BODY"
  echo
  echo "--- runtime output ---"
  "$BIN"
} > "$REPORT" 2>&1

fail=0

if [ -z "$STATIC_BODY" ]; then
  echo "FAIL: could not find run_static in generated assembly (see $REPORT)"
  fail=1
elif echo "$STATIC_BODY" | grep -Eq 'call[a-z]*\s+\*|jmp[a-z]*\s+\*'; then
  echo "FAIL: run_static contains an indirect call/jmp — composition no longer collapses (see $REPORT)"
  fail=1
else
  echo "OK: run_static is straight-line (no indirect call/jmp)"
fi

if [ -z "$DYNAMIC_BODY" ]; then
  echo "FAIL: could not find run_dynamic in generated assembly (see $REPORT)"
  fail=1
elif echo "$DYNAMIC_BODY" | grep -Eq 'call[a-z]*\s+\*|jmp[a-z]*\s+\*'; then
  echo "OK: run_dynamic still goes through a real indirect call/jmp (control case intact)"
else
  echo "FAIL: run_dynamic has no indirect call/jmp — the control case is broken, check IItem/opaque pointer setup (see $REPORT)"
  fail=1
fi

echo "full report: $REPORT"

exit $fail
