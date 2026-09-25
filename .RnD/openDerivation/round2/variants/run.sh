#!/bin/bash
# Round 2 variant: hapi::APIOf written in ':' syntax, `struct APIOf : (OO : ... : final API) {}`, is closed by the real
# hapi::APIOf (APIOf starts the Part collapse), so it derives from it and has the same base, Base and Types.
cd "$(dirname "$0")"
R=$(cd ../../../.. && pwd); H=${HAPI:-$R/include}
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
echo "== APIOf in ':' syntax"
python3 ../../translate.py --report apiof/apiof_od.cpp -o apiof/apiof_od.out.cpp
for c in g++ clang++; do $c -std=c++17 -I"$H" apiof/apiof_od.out.cpp -o "$W/a" && echo "  ok    apiof [$c]: static_asserts hold"; done
