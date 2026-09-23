#!/usr/bin/env bash
# Expect-to-fail tests. Each tests/negative/*.cpp must FAIL to compile AND its compiler output must contain the
# text on its "// EXPECT-ERROR:" line, so a case cannot start "passing" for the wrong reason (e.g. a typo).
# Usage: tests/negative/run.sh   (CXX=clang++ to use another compiler; exits non-zero on any mismatch)
set -u
cd "$(dirname "$0")"
CXX=${CXX:-g++}
FLAGS="-std=c++17 -fsyntax-only -I../../include"
bad=0; n=0
for f in *.cpp; do
  n=$((n+1))
  expect=$(sed -n 's|^// EXPECT-ERROR: *||p' "$f" | head -1)
  if [ -z "$expect" ]; then echo "BAD  $f: no '// EXPECT-ERROR:' line"; bad=$((bad+1)); continue; fi
  out=$($CXX $FLAGS "$f" 2>&1); rc=$?
  if [ $rc -eq 0 ]; then echo "FAIL $f: compiled, but an error was expected"; bad=$((bad+1))
  elif printf '%s' "$out" | grep -qF -- "$expect"; then echo "ok   $f"
  else echo "FAIL $f: failed to compile, but without the expected text: $expect"; bad=$((bad+1)); fi
done
echo "$((n-bad))/$n negative cases behave as expected ($CXX)"
[ "$bad" -eq 0 ]
