#!/usr/bin/env bash
# HAPI standalone benchmark runner
# Usage: ./run_bench.sh [CXX=clang++] [SIZES="10 25 50 100 200"] [REPS=100000]
#
# Measures:
#   compile time  — -fsyntax-only at N = 10 25 50 100 200
#   runtime       — forEach / runEach throughput at N = 50, 200
#   binary size   — stripped runtime binary
#
# Results are stored in benchmark/results/YYYY-MM-DD_HH-MM-SS.log
# and diffed against the previous run.

set -euo pipefail

BENCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HAPI_INCLUDE="${BENCH_DIR}/../include"
SRC="${BENCH_DIR}/bench_hapi.cpp"
RESULTS_DIR="${BENCH_DIR}/results"
mkdir -p "${RESULTS_DIR}"

TIMESTAMP=$(date '+%Y-%m-%d_%H-%M-%S')
LOG="${RESULTS_DIR}/${TIMESTAMP}.log"
TMP=$(mktemp -d)
trap 'rm -rf "${TMP}"' EXIT

: "${CXX:=g++}"
: "${SIZES:=10 25 50 100 200}"
: "${REPS:=100000}"
: "${RUNTIME_SIZES:=50 200}"

FLAGS="-std=c++17 -ftemplate-depth=2000 -I${HAPI_INCLUDE}"

# ── helpers ──────────────────────────────────────────────────────────────────

ms_now() { date +%s%3N; }

# compile with given flags, return elapsed ms or "ERR"
compile_ms() {
    local rc=0 t0 t1
    t0=$(ms_now)
    "${CXX}" ${FLAGS} "$@" "${SRC}" 2>/dev/null || rc=$?
    t1=$(ms_now)
    [[ ${rc} -eq 0 ]] && echo $((t1 - t0)) || echo "ERR"
}

# read RunInlineMax from run.h (first numeric match)
inline_max() {
    grep 'RunInlineMax = [0-9]' "${HAPI_INCLUDE}/hapi/run.h" \
        | grep -o '[0-9][0-9]*' | head -1 || echo "?"
}

# ── benchmark body (piped to tee) ────────────────────────────────────────────

{
    CXX_VER=$("${CXX}" --version | head -1)
    HAPI_VER=$(grep '"version"' "${BENCH_DIR}/../library.json" 2>/dev/null \
               | sed 's/.*"\([0-9][0-9.]*\)".*/\1/' || echo "?")

    echo "=== HAPI Standalone Benchmark ==="
    echo "Date:         $(date '+%Y-%m-%d %H:%M:%S')"
    echo "Host:         $(uname -n)"
    echo "Compiler:     ${CXX_VER}"
    echo "HAPI:         ${HAPI_VER}"
    echo "RunInlineMax: $(inline_max)  (runEach uses table for N > this)"
    echo "Flags:        ${FLAGS}"
    echo ""

    # ── compile-time table ────────────────────────────────────────────────────

    echo "--- Compile time (ms, -fsyntax-only -O0) ---"
    echo "  baseline: HAPI include + compiler startup, no Chain<N>"
    echo "  node_only - baseline ≈ pure Chain<N> instantiation cost"
    echo "  find_xxx  - node_only ≈ pure FindFirst traversal cost"
    echo "  foreach   - node_only ≈ forEach codegen cost"
    echo ""

    declare -a TESTS=( baseline map find_first find_mid find_last foreach runeach node_only at_array mapped )
    declare -a DEFS=(  TEST_BASELINE TEST_MAP TEST_FIND_FIRST TEST_FIND_MID TEST_FIND_LAST \
                       TEST_FOREACH TEST_RUNEACH TEST_NODE_ONLY TEST_AT_ARRAY TEST_MAPPED )
    read -ra SIZE_ARR <<< "${SIZES}"

    printf "%-14s" ""
    for N in "${SIZE_ARR[@]}"; do printf "%7s" "N=${N}"; done
    echo ""
    printf "%-14s" ""
    for N in "${SIZE_ARR[@]}"; do printf "%7s" "------"; done
    echo ""

    for i in "${!TESTS[@]}"; do
        printf "%-14s" "${TESTS[$i]}"
        for N in "${SIZE_ARR[@]}"; do
            ms=$(compile_ms -O0 -fsyntax-only "-DTEST_SIZE=${N}" "-D${DEFS[$i]}")
            printf "%7s" "${ms}"
        done
        echo ""
    done

    echo ""

    # ── runtime measurements ──────────────────────────────────────────────────

    echo "--- Runtime (-O2, REPS=${REPS}) ---"
    echo ""

    read -ra RT_SIZES <<< "${RUNTIME_SIZES}"
    for N in "${RT_SIZES[@]}"; do
        BIN="${TMP}/bench_rt_${N}"
        echo "  N=${N}:"
        if "${CXX}" ${FLAGS} -O2 -DTEST_RUNTIME "-DTEST_SIZE=${N}" "-DREPS=${REPS}" \
                   -o "${BIN}" "${SRC}" 2>/dev/null; then
            KB=$(wc -c < "${BIN}" | awk '{printf "%.1f", $1/1024}')
            printf "    binary size: %s KB\n" "${KB}"
            "${BIN}" | sed 's/^/    /'
        else
            echo "    [compile failed]"
        fi
        echo ""
    done

} | tee "${LOG}"

# ── compare with previous log ─────────────────────────────────────────────────

PREV=$(ls -1t "${RESULTS_DIR}"/*.log 2>/dev/null | grep -vF "${LOG}" | head -1 || true)
if [[ -n "${PREV}" ]]; then
    echo ""
    echo "=== Diff vs $(basename "${PREV}") ==="
    diff "${PREV}" "${LOG}" || true
fi

echo ""
echo "Saved: ${LOG}"
