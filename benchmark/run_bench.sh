#!/usr/bin/env bash
# HAPI vs Hana compile-time benchmark (all test categories)
# Usage: ./run_bench.sh [CXX=g++] [SIZES="10 25 50 100 200"] [TREE_SIZES="5 10 13"]
#
# Generates bench_compile.png with 4 panels:
#   top-left:     Map type-level        (flat chain, N elements)
#   top-right:    FindFirst first/mid/last (flat chain)
#   bottom-left:  Tree topology B×B
#   bottom-right: Value-level iteration vs type-level forEach

set -euo pipefail

BENCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HAPI_INCLUDE="${BENCH_DIR}/../include"
SRC="${BENCH_DIR}/main.cpp"
RESULTS_DIR="${BENCH_DIR}/results"
mkdir -p "${RESULTS_DIR}"

TIMESTAMP=$(date '+%Y-%m-%d_%H-%M-%S')
LOG="${RESULTS_DIR}/${TIMESTAMP}.log"

: "${CXX:=g++}"
: "${SIZES:=10 25 50 100 200}"
: "${TREE_SIZES:=5 7 10 13}"

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

    echo "=== HAPI vs Hana Compile-time Benchmark ==="
    echo "Date:     $(date '+%Y-%m-%d %H:%M:%S')"
    echo "Host:     $(uname -n)"
    echo "Compiler: ${CXX_VER}"
    echo "HAPI:     ${HAPI_VER}"
    echo "Flags:    ${FLAGS}"
    echo ""

    read -ra SIZE_ARR <<< "${SIZES}"
    read -ra TREE_ARR <<< "${TREE_SIZES}"

    # ── flat-N section ────────────────────────────────────────────────────────

    echo "### SECTION: flat ###"
    printf "%-20s" ""
    for N in "${SIZE_ARR[@]}"; do printf "%7s" "N=${N}"; done
    echo ""
    printf "%-20s" ""
    for N in "${SIZE_ARR[@]}"; do printf "%7s" "------"; done
    echo ""

    declare -a FLAT_LABELS=( baseline
                             hapi_type    hana_type
                             hapi_first   hana_first
                             hapi_mid     hana_mid
                             hapi_last    hana_last
                             hana_val_map hana_val_find )
    declare -a FLAT_DEFS=(   TEST_BASELINE
                             TEST_HAPI_TYPE    TEST_HANA_TYPE
                             TEST_HAPI_FIRST   TEST_HANA_FIRST
                             TEST_HAPI_MIDDLE  TEST_HANA_MIDDLE
                             TEST_HAPI_LAST    TEST_HANA_LAST
                             TEST_HANA_VAL_MAP TEST_HANA_VAL_FIND )

    for i in "${!FLAT_LABELS[@]}"; do
        printf "%-20s" "${FLAT_LABELS[$i]}"
        for N in "${SIZE_ARR[@]}"; do
            ms=$(compile_ms -O0 -fsyntax-only "-DTEST_SIZE=${N}" "-D${FLAT_DEFS[$i]}")
            printf "%7s" "${ms}"
        done
        echo ""
        case "${FLAT_LABELS[$i]}" in hana_type|hana_first|hana_mid|hana_last)
            echo ""
        esac
    done

    echo ""

    # ── tree-B section ────────────────────────────────────────────────────────

    echo "### SECTION: tree ###"
    printf "%-20s" ""
    for B in "${TREE_ARR[@]}"; do printf "%7s" "B=${B}"; done
    echo ""
    printf "%-20s" ""
    for B in "${TREE_ARR[@]}"; do printf "%7s" "------"; done
    echo ""

    declare -a TREE_LABELS=( baseline
                             hapi_tree_map hapi_tree_first hapi_tree_last
                             hana_tree_map hana_tree_find )
    declare -a TREE_DEFS=(   TEST_BASELINE
                             TEST_HAPI_TREE_MAP TEST_HAPI_TREE_FIRST TEST_HAPI_TREE_LAST
                             TEST_HANA_TREE_MAP TEST_HANA_TREE_FIND )

    for i in "${!TREE_LABELS[@]}"; do
        printf "%-20s" "${TREE_LABELS[$i]}"
        for B in "${TREE_ARR[@]}"; do
            ms=$(compile_ms -O0 -fsyntax-only "-DTREE_B=${B}" "-D${TREE_DEFS[$i]}")
            printf "%7s" "${ms}"
        done
        echo ""
        case "${TREE_LABELS[$i]}" in baseline|hapi_tree_last)
            echo ""
        esac
    done

} | tee "${LOG}"

PREV=$(ls -1t "${RESULTS_DIR}"/*.log 2>/dev/null | grep -vF "${LOG}" | head -1 || true)
if [[ -n "${PREV}" ]]; then
    echo ""
    echo "=== Diff vs $(basename "${PREV}") ==="
    diff "${PREV}" "${LOG}" || true
fi

echo ""
echo "Saved: ${LOG}"

python3 "${BENCH_DIR}/bench_chart.py" "${LOG}"
