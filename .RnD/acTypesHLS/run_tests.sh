#!/bin/bash
# AC Datatypes (HLSLibs ac_types) reusability test suite.
#
# HAPI has no existing test-runner/CI convention to plug into (tests/
# compile_tests.cpp is a single hand-compiled file, no script) -- this is
# new, scoped scaffolding for this investigation only, not a proposal for
# permanent HAPI test infrastructure.
#
# Requires:
#   BAMBU_APPIMAGE     - path to a bambu AppImage
#                        (https://release.bambuhls.eu/bambu-2024.10.AppImage)
#   AC_TYPES_INCLUDE   - path to a clone's include/ dir
#                        (git clone --depth 1 https://github.com/hlslibs/ac_types)
#
# Exits non-zero if any check fails or gives an unexpected result.
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HAPI_INC="$HERE/../../include"
DEVICE=xc7a100t-1csg324-VVD
CLOCK=10
FAIL=0

if [ -z "${BAMBU_APPIMAGE:-}" ]; then
  echo "FAIL: BAMBU_APPIMAGE not set"; exit 1
fi
if [ -z "${AC_TYPES_INCLUDE:-}" ]; then
  echo "FAIL: AC_TYPES_INCLUDE not set"; exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

pass() { echo "PASS: $1"; }
fail() { echo "FAIL: $1"; FAIL=1; }

# --- 1. Native functional correctness (no Bambu) ---------------------------
g++ -std=gnu++17 -I"$AC_TYPES_INCLUDE" "$HERE/tests/ac_types_native_test.cpp" \
  -o "$WORK/native_test" 2>"$WORK/native_build.log"
if [ $? -eq 0 ] && "$WORK/native_test" >"$WORK/native_run.log" 2>&1; then
  pass "native ac_int/ac_fixed correctness (assert-checked known values)"
else
  fail "native correctness test (see build/run log below)"
  cat "$WORK/native_build.log" "$WORK/native_run.log"
fi

# Reg<T> shell: linked directly against the real hls/*.cpp targets (not a
# reimplementation), including the public examples/hls_fir baseline file,
# to compare its raw-member output against the shell-based rewrite.
g++ -std=gnu++17 -I"$AC_TYPES_INCLUDE" -I"$HAPI_INC" \
  "$HERE/tests/hapi_reg_native_test.cpp" \
  "$HERE/../../examples/hls_fir/hls/fir_lpf4_actypes_top.cpp" \
  "$HERE/hls/fir_lpf4_actypes_reg_top.cpp" \
  "$HERE/hls/ac_accumulator_top.cpp" \
  "$HERE/hls/ac_complex_cmac_top.cpp" \
  -o "$WORK/reg_native_test" 2>"$WORK/reg_native_build.log"
if [ $? -eq 0 ] && "$WORK/reg_native_test" >"$WORK/reg_native_run.log" 2>&1; then
  pass "native Reg<T> correctness (FIR raw-vs-shell, accumulator, complex-mac)"
else
  fail "native Reg<T> test (see build/run log below)"
  cat "$WORK/reg_native_build.log" "$WORK/reg_native_run.log"
fi

# Biquad IIR: Reg<T> used for both feedforward and feedback taps.
g++ -std=gnu++17 -I"$AC_TYPES_INCLUDE" -I"$HAPI_INC" \
  "$HERE/tests/biquad_native_test.cpp" \
  "$HERE/hls/biquad_top.cpp" \
  "$HERE/hls/biquad_cascade2_top.cpp" \
  -o "$WORK/biquad_native_test" 2>"$WORK/biquad_native_build.log"
if [ $? -eq 0 ] && "$WORK/biquad_native_test" >"$WORK/biquad_native_run.log" 2>&1; then
  pass "native biquad IIR correctness (single section + cascade convolution check)"
else
  fail "native biquad test (see build/run log below)"
  cat "$WORK/biquad_native_build.log" "$WORK/biquad_native_run.log"
fi

# --- 2. Negative control: guard must reject Bambu's own bundled fork -------
BUNDLED_EXTRACT="$WORK/bambu_extract"
mkdir -p "$BUNDLED_EXTRACT"
(cd "$BUNDLED_EXTRACT" && "$BAMBU_APPIMAGE" --appimage-extract >/dev/null 2>&1)
BUNDLED_INC="$BUNDLED_EXTRACT/squashfs-root/usr/share/panda/ac_types/include"
if [ -d "$BUNDLED_INC" ]; then
  g++ -std=gnu++17 -I"$BUNDLED_INC" "$HERE/tests/ac_types_native_test.cpp" \
    -o "$WORK/bundled_test" 2>"$WORK/bundled_build.log"
  if [ $? -ne 0 ] && grep -q "resolved to bambu's bundled ac_types fork" "$WORK/bundled_build.log"; then
    pass "AC_VERSION guard correctly rejects bambu's bundled fork (negative control)"
  else
    fail "guard did NOT reject bambu's bundled fork -- silent-fallback trap is back"
  fi
else
  fail "could not locate bambu's bundled ac_types under the extracted AppImage (layout changed?)"
fi

# --- 3. Synthesis path: each target, single naive -I, must pass the guard --
run_bambu() {
  local name="$1" topfn="$2" src="$3"; shift 3
  local outdir="$WORK/hls_$name"
  mkdir -p "$outdir"
  ( cd "$outdir" && "$BAMBU_APPIMAGE" "$@" --std=gnu++17 --compiler=I386_CLANG16 \
      --device-name=$DEVICE --clock-period=$CLOCK \
      --top-fname="$topfn" -v2 "$src" > bambu.log 2>&1 )
  echo $?
}

for t in \
  "fir_lpf4_actypes:firLpf4ActypesTop:$HERE/../../examples/hls_fir/hls/fir_lpf4_actypes_top.cpp" \
  "ac_int_ops:acIntOpsTop:$HERE/hls/ac_int_ops_top.cpp" \
  "ac_fixed_frac:acFixedFracTop:$HERE/hls/ac_fixed_frac_top.cpp" \
  "fir_lpf4_actypes_reg:firLpf4ActypesRegTop:$HERE/hls/fir_lpf4_actypes_reg_top.cpp" \
  "ac_accumulator:acAccumulatorTop:$HERE/hls/ac_accumulator_top.cpp" \
  "ac_complex_cmac:cmacTop:$HERE/hls/ac_complex_cmac_top.cpp" \
  "biquad:biquadTop:$HERE/hls/biquad_top.cpp" \
  "biquad_cascade2:biquadCascade2Top:$HERE/hls/biquad_cascade2_top.cpp" \
; do
  IFS=: read -r name topfn src <<< "$t"
  rc=$(run_bambu "$name" "$topfn" "$src" -I"$AC_TYPES_INCLUDE" -I"$HAPI_INC")
  # ac_complex_cmac takes pointer out-params (int32_t* out_re, out_im) --
  # the only pointer-arg target in this suite -- and Bambu emits one
  # expected, benign informational warning about the resulting
  # runtime-address memory port ("unknown addresses"), not a defect.
  # Any OTHER warning still fails the check.
  warnings=$(grep -i "warning" "$WORK/hls_$name/bambu.log")
  if [ "$name" = "ac_complex_cmac" ]; then
    warnings=$(echo "$warnings" | grep -v "unknown addresses")
  fi
  if [ "$rc" = "0" ] && ! grep -qi "error" "$WORK/hls_$name/bambu.log" && [ -z "$warnings" ]; then
    pass "synthesize $name (naive -I order, real upstream header confirmed by guard)"
  else
    fail "synthesize $name (exit=$rc, or unexpected warnings/errors present)"
    grep -in "error\|warning" "$WORK/hls_$name/bambu.log" | head -10
  fi
done

# --- 4. Include-order matrix on the FIR case: does order actually matter? --
FIR_SRC="$HERE/../../examples/hls_fir/hls/fir_lpf4_actypes_top.cpp"

rc=$(run_bambu fir_naive firLpf4ActypesTop "$FIR_SRC" -I"$AC_TYPES_INCLUDE" -I"$HAPI_INC")
[ "$rc" = "0" ] && pass "order (i) ac_types-first: succeeds" || fail "order (i) ac_types-first: unexpectedly failed"

rc=$(run_bambu fir_reversed firLpf4ActypesTop "$FIR_SRC" -I"$HAPI_INC" -I"$AC_TYPES_INCLUDE")
[ "$rc" = "0" ] && pass "order (ii) ac_types-last: succeeds (ordering does not matter)" || fail "order (ii) ac_types-last: unexpectedly failed"

if [ "$(grep -c '' "$WORK/hls_fir_naive/bambu.log" 2>/dev/null)" -gt 0 ] && \
   [ "$(grep -c '' "$WORK/hls_fir_reversed/bambu.log" 2>/dev/null)" -gt 0 ]; then
  n1=$(grep "Total number of flip-flops" "$WORK/hls_fir_naive/bambu.log")
  n2=$(grep "Total number of flip-flops" "$WORK/hls_fir_reversed/bambu.log")
  if [ "$n1" = "$n2" ]; then
    pass "order (i) and (ii) produce identical resource numbers"
  else
    fail "order (i) and (ii) produce DIFFERENT numbers -- ordering matters after all"
  fi
fi

rc=$(run_bambu fir_omitted firLpf4ActypesTop "$FIR_SRC" -I"$HAPI_INC")
if [ "$rc" != "0" ] && grep -q "resolved to bambu's bundled ac_types fork" "$WORK/hls_fir_omitted/bambu.log"; then
  pass "order (iii) omitted -I: fails LOUDLY via the guard (not a silent wrong-numbers pass)"
else
  fail "order (iii) omitted -I: did not fail the way the guard should force it to"
fi

# --- 5. Zero-overhead diff: raw-member baseline vs. Reg<T> shell -----------
# Both were already synthesized in step 3 (fir_lpf4_actypes,
# fir_lpf4_actypes_reg) -- diff their resource counts directly instead of
# eyeballing two log files.
metrics() {
  grep -E "Total number of flip-flops|Total estimated area|Estimated number of DSPs|Number of control steps|Register allocation algorithm" "$1"
}
m1="$(metrics "$WORK/hls_fir_lpf4_actypes/bambu.log")"
m2="$(metrics "$WORK/hls_fir_lpf4_actypes_reg/bambu.log")"
# Function names differ (firLpf4ActypesTop vs firLpf4ActypesRegTop) so the
# FF-count line's mangled suffix differs too -- strip trailing identifiers
# before comparing, everything else must match verbatim.
m1n="$(echo "$m1" | sed -E 's/_Z[0-9A-Za-z_]+//g')"
m2n="$(echo "$m2" | sed -E 's/_Z[0-9A-Za-z_]+//g')"
if [ -n "$m1n" ] && [ "$m1n" = "$m2n" ]; then
  pass "zero-overhead: raw-member baseline and Reg<T> shell produce identical FF/registers/area/DSP/control-steps"
else
  fail "zero-overhead claim broken -- baseline and shell differ:"
  diff <(echo "$m1") <(echo "$m2")
fi

echo
if [ "$FAIL" -eq 0 ]; then
  echo "ALL CHECKS PASS"
else
  echo "ONE OR MORE CHECKS FAILED -- see above"
fi
exit $FAIL
