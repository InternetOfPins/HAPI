#!/usr/bin/env bash
# CI guard: HAPI's Chain/APIOf collapse must keep synthesizing through
# Bambu, and the resulting hardware cost must stay flat as layers are
# added -- not proof of correctness, just a tripwire against silent
# regressions (either in HAPI or in a Bambu upgrade).
#
# Usage: run_hls_smoke.sh <path-to-bambu.AppImage> <path-to-HAPI/include>
set -euo pipefail

BAMBU="${1:?path to bambu AppImage required}"
HAPI_INCLUDE="${2:?path to HAPI/include required}"
SRC="$(cd "$(dirname "$0")" && pwd)/src/main.cpp"
OUTDIR="$(mktemp -d)"

echo "== compile-time guards (static_assert) =="
# Fails fast, no Bambu run needed, if the type-level regressions trip.
clang++ -std=gnu++17 -I"$HAPI_INCLUDE" -fsyntax-only "$SRC"
echo "OK: static_assert guards passed"

echo "== Bambu synthesis =="
cd "$OUTDIR"
# Same explicit device/period as hls_fir/hls_can_disabler (extra_hls.py) --
# an unconfirmed/default device produces numbers not citable against any
# real, ownable board. See README.md's Target device note.
"$BAMBU" -I"$HAPI_INCLUDE" --std=gnu++17 --compiler=I386_CLANG16 \
  --device-name=xc7a100t-1csg324-VVD --clock-period=10 \
  --top-fname=wrapSum -v2 "$SRC" 2>&1 | tee bambu.log

if [ ! -f wrapSum.v ]; then
  echo "FAIL: no RTL produced (wrapSum.v missing)"
  exit 1
fi

echo "== adder-count check =="
# Read Bambu's own resource summary, not a grep over the generated Verilog:
# a single real ui_plus_expr_FU instance shows up ~5x as a raw substring in
# the .v file (module definition, wire name, instantiation line, parameter
# continuation, assign statement), so counting text occurrences is not a
# valid proxy for functional-unit count. The "Summary of resources:" block
# is Bambu's own accounting and is what actually reflects synthesized FUs.
ADDER_COUNT=$(awk '/Summary of resources:/{f=1} f && /ui_plus_expr_FU:/{print $NF}' bambu.log)
echo "adder instances (per Bambu's resource summary): ${ADDER_COUNT:-0}"
if [ "${ADDER_COUNT:-0}" -ne 1 ]; then
  echo "FAIL: expected exactly 1 folded adder, found ${ADDER_COUNT:-0} -- Chain collapse may have regressed"
  exit 1
fi

echo "PASS: HAPI HLS smoke test clean (guards + synthesis + adder count)"
