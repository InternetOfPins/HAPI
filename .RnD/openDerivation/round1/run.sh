#!/bin/bash
# Round 1: translate src/ -> out/, build natively (g++, clang++, -std=c++17), run; bare use of an open class must not compile.
cd "$(dirname "$0")"
H=${HAPI:-../../../include}
pass=0; fail=0
ok()  { echo "  ok    $1"; pass=$((pass+1)); }
bad() { echo "  FAIL  $1: $2"; fail=$((fail+1)); }
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
python3 ../translate.py --report --outdir out src/*.cpp || { echo "translation failed"; exit 1; }
for cxx in g++ clang++; do command -v $cxx >/dev/null || continue
  if $cxx -std=c++17 -Wall -Wextra -I"$H" out/minimal.cpp -o "$W/m" 2>"$W/err"; then
    o=$("$W/m"); rc=$?
    [ $rc -eq 0 ] && ok "minimal [$cxx]: $o" || bad "minimal [$cxx]" "exit $rc: $o"
  else bad "minimal [$cxx]" "$(grep -m1 error "$W/err")"; fi
  if $cxx -std=c++17 -fsyntax-only -I"$H" out/bare_use.cpp 2>"$W/err"; then bad "bare_use [$cxx]" "compiled, must be rejected"
  else ok "bare_use [$cxx]: rejected: $(grep -m1 error "$W/err" | sed 's/^[^ ]* //')"; fi
done
echo; echo "$pass ok, $fail FAIL"; [ $fail -eq 0 ]
