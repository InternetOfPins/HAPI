#!/usr/bin/env bash
# Full reproducer with plain g++ (no PlatformIO): build both velocity-method
# variants, disassemble CartToJnt to show the dispatch fork, run the bit-exact
# loopback per solver family (KDL's shipped ChainIkSolverPos_NR / _NR_JL vs our
# Ref vs our Hapi).
#
# Needs a local KDL -- run ./setup_kdl.sh first, or point KDL_DIR at one.
set -euo pipefail
cd "$(dirname "$0")"

KDL=${KDL_DIR:-$HOME/kdl}
INSTALL=$KDL/install
[ -f "$INSTALL/lib/liborocos-kdl.so" ] || { echo "no KDL at $INSTALL -- run ./setup_kdl.sh"; exit 1; }

EIG=${EIGEN_DIR:-}
if [ -z "$EIG" ]; then
  if [ -f "$KDL/eigen/Eigen/Core" ]; then EIG=$KDL/eigen
  elif [ -f /usr/include/eigen3/Eigen/Core ]; then EIG=/usr/include/eigen3
  else EIG=$(pkg-config --cflags-only-I eigen3 2>/dev/null | sed 's/-I//' || true); fi
fi
[ -n "$EIG" ] && [ -f "$EIG/Eigen/Core" ] || { echo "no Eigen -- set EIGEN_DIR"; exit 1; }

INC="-isystem $INSTALL/include -isystem $EIG -I../../include"
LIB="-L$INSTALL/lib -Wl,-rpath,$INSTALL/lib -lorocos-kdl"
STD="-std=c++17 -O2 -Wno-deprecated-copy"
O=$(mktemp -d); trap 'rm -rf "$O"' EXIT

echo "### build + run  (pinv, wdls)"
for v in "pinv:" "wdls:-DWDLS"; do
  name=${v%%:*}; flag=${v#*:}
  g++ $STD $INC $flag src/kdlCompose.cpp -o "$O/kc_$name" $LIB
  g++ $STD $INC $flag -c src/kdlCompose.cpp -o "$O/kc_$name.o"
  "$O/kc_$name"
  echo
done

echo "### objdump -- CartToJnt loop body, call instructions + relocations only"
echo "###   Ref  = KDL's loop, ChainFkSolverPos& / ChainIkSolverVel&  (== shipped shape)"
echo "###   Hapi = same loop, sub-solvers are compile-time hapi::Chain<> stages"
slice() {
  objdump -dCr "$O/$1" | \
    awk -v s="$2" 'index($0,s"(")&&/>:$/{f=1} f&&/^$/{exit} f' | \
    grep -E 'call[[:space:]]|R_X86_64_(PLT32|PC32)' || true
}
for sym in "kdlCompose::IkSolverPos_NR_Ref::CartToJnt" \
           "kdlCompose::IkSolverPos_NR_Hapi<(kdlCompose::VelMethod)0>::CartToJnt" \
           "kdlCompose::IkSolverPos_NR_JL_Ref::CartToJnt" \
           "kdlCompose::IkSolverPos_NR_JL_Hapi<(kdlCompose::VelMethod)0>::CartToJnt"; do
  echo "--- ${sym#kdlCompose::} ---"
  body=$(slice kc_pinv.o "$sym")
  printf '%s\n' "$body"
  n=$(printf '%s\n' "$body" | grep -cE 'call[[:space:]]+\*' || true)
  echo "    indirect (vtable) calls in loop body: $n"
done
echo
echo "expected: each Ref has 2 indirect  'call   *0xNN(%rax)'  (fk / ik vtable);"
echo "          each Hapi has 0 -- both became direct 'call <KDL::...JntToCart / ...CartToJnt>'."
