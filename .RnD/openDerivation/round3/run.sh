#!/bin/bash
# Round 3: coverage, struct-only form (':' only in base clauses). Every program in src/pos is translated with the default
# chain lowering (HAPI's form) and, as an EXPERIMENTAL extra, --lower=nested;
# built with g++ and clang++ (-std=c++17 -Wall) and run; src/neg_translate must be refused by the translator with the
# diagnostic on its `// expect:` line; src/neg_compile must translate, then be rejected by both compilers (expect = regex).
cd "$(dirname "$0")"
H=${HAPI:-../../../include}
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
pass=0; fail=0
ok()  { echo "  ok    $1"; pass=$((pass+1)); }
bad() { echo "  FAIL  $1: $2"; fail=$((fail+1)); }
cxxs() { for c in g++ clang++; do command -v $c >/dev/null && echo $c; done; }

for mode in chain nested; do
  [ $mode = chain ] && echo "== positive, --lower=chain" || echo "== positive, --lower=nested (EXPERIMENTAL)"
  o=out/$mode; rm -rf "$o"; mkdir -p "$o"
  python3 ../translate.py --lower=$mode --report --outdir "$o" src/pos/* 2> "$o/translate.log" || { bad "translate [$mode]" "$(cat "$o/translate.log")"; continue; }
  D=; [ $mode = nested ] && D=-DOD_NESTED
  for c in $(cxxs); do
    for f in base_chain named_chain open_terminal distinct_args ctors self family identity user_super dependent label_hazard; do
      if $c -std=c++17 -Wall -Wno-unused-label $D -I"$H" -I../support -I"$o" "$o/$f.cpp" -o "$W/x" 2>"$W/err"; then
        r=$("$W/x"); [ $? -eq 0 ] && ok "$f [$mode, $c]: $r" || bad "$f [$mode, $c]" "$r"
      else bad "$f [$mode, $c]" "$(grep -m1 -E 'error|static assert' "$W/err")"; fi
    done
    if $c -std=c++17 -Wall $D -I"$H" -I../support -I"$o" "$o/identity_tu1.cpp" "$o/identity_tu2.cpp" -o "$W/x" 2>"$W/err"; then
      r=$("$W/x"); [ $? -eq 0 ] && ok "identity_tu [$mode, $c]: $r" || bad "identity_tu [$mode, $c]" "$r"
    else bad "identity_tu [$mode, $c]" "$(grep -m1 -E 'error|undefined' "$W/err")"; fi
  done
done

echo "== the translator must refuse"
for f in src/neg_translate/*.cpp; do n=$(basename "$f" .cpp); e=$(head -1 "$f" | sed 's|^// expect: ||')
  if python3 ../translate.py "$f" > /dev/null 2> "$W/err"; then bad "$n" "translated, but must be refused"
  elif grep -qF -- "$e" "$W/err"; then ok "$n: $(head -1 "$W/err" | sed 's|^src/||')"
  else bad "$n" "refused, but not with \"$e\": $(head -1 "$W/err")"; fi
done

echo "== the compilers must reject (after translation; any compiler error is accepted)"
rm -rf out/neg_compile; mkdir -p out/neg_compile
for f in src/neg_compile/*.cpp; do n=$(basename "$f" .cpp); e=$(head -1 "$f" | sed 's|^// expect: ||')
  python3 ../translate.py "$f" -o out/neg_compile/$n.cpp || { bad "$n" "translation failed"; continue; }
  for c in $(cxxs); do
    if $c -std=c++17 -fsyntax-only -I"$H" -I../support out/neg_compile/$n.cpp 2>"$W/err"; then bad "$n [$c]" "compiled, but must be rejected"
    else m=$(grep -m1 'error' "$W/err" | sed 's|^[^ ]* ||')
      printf '%s' "$m" | grep -qE -- "$e" && ok "$n [$c]: $m" || bad "$n [$c]" "rejected, but not with /$e/: $m"; fi
  done
done
echo; echo "$pass ok, $fail FAIL"; [ $fail -eq 0 ]
