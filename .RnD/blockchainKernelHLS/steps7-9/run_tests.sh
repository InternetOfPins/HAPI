#!/usr/bin/env bash
# steps 7-9 of the blockchain-as-HAPI-composition experiment -- see ../HANDOFF.md
# Usage: ./run_tests.sh /path/to/HAPI/include
set -euo pipefail

HAPI_INC="${1:?path to HAPI/include required}"
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

pass() { echo "PASS: $1"; }
fail() { echo "FAIL: $1"; exit 1; }

echo "== step 8: heterogeneous composition, sizeof(ChainA)==12 =="
cat > "$WORK/sizeof_check.cpp" <<EOF
#include "$HERE/components_portable.h"
#include <cstdio>
using ChainA = hapi::APIOf<BlockAPI<>, SimpleConsensus, Validation, Hash, Transaction, State>;
int main() { printf("%zu\n", sizeof(ChainA)); return 0; }
EOF
g++ -std=c++17 -O0 -I "$HAPI_INC" "$WORK/sizeof_check.cpp" -o "$WORK/sizeof_check"
sz=$("$WORK/sizeof_check")
[ "$sz" = "12" ] && pass "sizeof(ChainA) == 12" || fail "sizeof(ChainA) == $sz, expected 12"

echo "== step 7: naive vs guarded missing-Hash diagnostics =="
if g++ -std=c++17 -fmax-errors=1 -I "$HAPI_INC" -c "$HERE/naive_missing_hash.cpp" -o "$WORK/naive.o" 2>"$WORK/naive.log"; then
  fail "naive_missing_hash.cpp compiled -- should fail"
fi
grep -q "has no member named .hash." "$WORK/naive.log" && pass "naive: raw member-lookup error" \
  || fail "naive: expected raw member-lookup error, got:\n$(cat "$WORK/naive.log")"

if g++ -std=c++17 -fmax-errors=1 -I "$HAPI_INC" -c "$HERE/guarded_missing_hash.cpp" -o "$WORK/guarded.o" 2>"$WORK/guarded.log"; then
  fail "guarded_missing_hash.cpp compiled -- should fail"
fi
grep -q "Hash must be reachable below Validation" "$WORK/guarded.log" && pass "guarded: actionable rules() message" \
  || fail "guarded: expected actionable message, got:\n$(cat "$WORK/guarded.log")"

echo "== step 9: shared-prefix symbol identity + linked-size deltas =="
g++ -std=c++17 -O0 -I "$HAPI_INC" -c "$HERE/chainA_tu_portable.cpp" -o "$WORK/chainA.o"
g++ -std=c++17 -O0 -I "$HAPI_INC" -c "$HERE/chainB_tu_portable.cpp" -o "$WORK/chainB.o"
g++ -std=c++17 -O0 -I "$HAPI_INC" -c "$HERE/chainA_crtp_tu.cpp"     -o "$WORK/chainA2.o"
g++ -std=c++17 -O0 -I "$HAPI_INC" -c "$HERE/chainB_crtp_tu.cpp"     -o "$WORK/chainB2.o"
echo 'int main(){return 0;}' > "$WORK/main.cpp"
g++ -std=c++17 -O0 -I "$HAPI_INC" -c "$WORK/main.cpp" -o "$WORK/main.o"

sym() { nm -C "$1" | grep -E "validate\(\) const|hash\(\) const|payload\(\) const" | sed 's/^[0-9a-f]* [A-Za-z] //'; }
if diff <(sym "$WORK/chainA.o") <(sym "$WORK/chainB.o") >/dev/null; then
  pass "no-CRTP: shared-prefix symbols byte-identical across chains"
else
  fail "no-CRTP: shared-prefix symbols differ (expected identical)"
fi
if diff <(sym "$WORK/chainA2.o") <(sym "$WORK/chainB2.o") >/dev/null; then
  fail "CRTP: shared-prefix symbols identical (expected them to differ -- terminal poisons type identity)"
else
  pass "CRTP: shared-prefix symbols now differ (terminal self-reference poisons the whole prefix)"
fi

text_of() { size "$1" | awk 'NR==2{print $1}'; }
g++ "$WORK/chainA.o" "$WORK/main.o" -o "$WORK/linkedA"
g++ "$WORK/chainA.o" "$WORK/chainB.o" "$WORK/main.o" -o "$WORK/linkedAB"
g++ "$WORK/chainA2.o" "$WORK/main.o" -o "$WORK/linkedA2"
g++ "$WORK/chainA2.o" "$WORK/chainB2.o" "$WORK/main.o" -o "$WORK/linkedA2B2"
deltaAB=$(( $(text_of "$WORK/linkedAB") - $(text_of "$WORK/linkedA") ))
deltaA2B2=$(( $(text_of "$WORK/linkedA2B2") - $(text_of "$WORK/linkedA2") ))
standaloneB2=$(text_of "$WORK/chainB2.o")
echo "  no-CRTP linked delta:   ${deltaAB}B  (small -- shared prefix, only Consensus duplicates)"
echo "  CRTP linked delta:      ${deltaA2B2}B  (chainB2.o standalone: ${standaloneB2}B -- should be close)"
[ "$deltaAB" -lt 400 ] && pass "no-CRTP linked delta stays small (<400B)" || fail "no-CRTP linked delta $deltaAB B, expected <400B"
[ "$deltaA2B2" -gt 500 ] && pass "CRTP linked delta is near-full duplication (>500B)" || fail "CRTP linked delta $deltaA2B2 B, expected >500B"

echo
echo "ALL PASS"
