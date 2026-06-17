#!/usr/bin/env bash
# HAPIFold benchmark runner — Mutate<F> and Ref<T,F> pipelines
# Usage: ./run_fold.sh [CXX=g++] [SIZES="10 25 50 100"] [REPS=1000000]

set -euo pipefail

BENCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HAPI_INCLUDE="${BENCH_DIR}/../include"
SRC="${BENCH_DIR}/bench_fold.cpp"
RESULTS_DIR="${BENCH_DIR}/results"
mkdir -p "${RESULTS_DIR}"

TIMESTAMP=$(date '+%Y-%m-%d_%H-%M-%S')
LOG="${RESULTS_DIR}/fold_${TIMESTAMP}.log"
TMP=$(mktemp -d)
trap 'rm -rf "${TMP}"' EXIT

: "${CXX:=g++}"
: "${SIZES:=10 25 50 100}"
: "${REPS:=1000000}"
: "${RUNTIME_SIZES:=10 50}"

FLAGS="-std=c++20 -ftemplate-depth=2000 -I${HAPI_INCLUDE}"

ms_now() { date +%s%3N; }

compile_ms() {
    local rc=0 t0 t1
    t0=$(ms_now)
    "${CXX}" ${FLAGS} "$@" "${SRC}" 2>/dev/null || rc=$?
    t1=$(ms_now)
    [[ ${rc} -eq 0 ]] && echo $((t1 - t0)) || echo "ERR"
}

{
    CXX_VER=$("${CXX}" --version | head -1)
    HAPI_VER=$(grep '"version"' "${BENCH_DIR}/../library.json" 2>/dev/null \
               | sed 's/.*"\([0-9][0-9.]*\)".*/\1/' || echo "?")

    echo "=== HAPIFold Benchmark ==="
    echo "Date:     $(date '+%Y-%m-%d %H:%M:%S')"
    echo "Host:     $(uname -n)"
    echo "Compiler: ${CXX_VER}"
    echo "HAPI:     ${HAPI_VER}"
    echo "Flags:    ${FLAGS}"
    echo ""

    # ── compile-time table ────────────────────────────────────────────────────

    echo "--- Compile time (ms, -fsyntax-only -O0) ---"
    printf "%-12s" "N"
    for TEST in baseline mutate ref trans; do
        printf "%10s" "${TEST}"
    done
    echo ""

    read -ra SIZE_ARR <<< "${SIZES}"

    for N in "${SIZE_ARR[@]}"; do
        printf "%-12s" "${N}"
        for pair in "baseline:TEST_BASELINE" "mutate:TEST_MUTATE" "ref:TEST_REF" "trans:TEST_TRANS"; do
            NAME="${pair%%:*}"; DEF="${pair##*:}"
            T=$(compile_ms -fsyntax-only -O0 "-DTEST_SIZE=${N}" "-D${DEF}")
            printf "%10s" "${T}"
        done
        echo ""
    done

    echo ""

    # ── runtime ──────────────────────────────────────────────────────────────

    echo "--- Runtime (ns/call, -O2) ---"

    for N in ${RUNTIME_SIZES}; do
        BIN="${TMP}/fold_runtime_${N}"
        echo -n "  building N=${N}... "
        if "${CXX}" ${FLAGS} -O2 "-DTEST_SIZE=${N}" "-DREPS=${REPS}" \
                -DTEST_FOLD_RUNTIME -o "${BIN}" "${SRC}" 2>/dev/null; then
            echo "ok"
            "${BIN}"
        else
            echo "FAILED"
            "${CXX}" ${FLAGS} -O2 "-DTEST_SIZE=${N}" "-DREPS=${REPS}" \
                -DTEST_FOLD_RUNTIME -o "${BIN}" "${SRC}" || true
        fi
        echo ""
    done

} 2>&1 | tee "${LOG}"

echo ""
echo "Results saved to: ${LOG}"
