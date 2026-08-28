#!/usr/bin/env bash
# Native correctness build: HAPI Chain<> output == TFLite-Micro output on a
# sine sweep. Compiles the minimal TFLM source set (check/tflm_srcs.txt --
# interpreter + allocator + FullyConnected only). Needs lib/tflm/ (run
# tools/get_tflm.sh first). TFLM objects cached in check/obj/.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
EX="$(dirname "$HERE")"
TFLM="$EX/lib/tflm"
HAPI_INC="$EX/../../include"
OBJ="$HERE/obj"; mkdir -p "$OBJ"

[ -d "$TFLM/tensorflow" ] || { echo "lib/tflm missing -- run tools/get_tflm.sh"; exit 1; }

INC="-I$EX -I$TFLM -I$TFLM/third_party/flatbuffers/include \
     -I$TFLM/third_party/gemmlowp -I$TFLM/third_party/ruy -I$HAPI_INC"
CXXFLAGS="-std=c++17 -O2 -fno-exceptions -fno-rtti -fno-threadsafe-statics \
     -DTF_LITE_STATIC_MEMORY -DTF_LITE_DISABLE_X86_NEON"

OBJS=()
while read -r rel; do
  [ -z "$rel" ] && continue
  o="$OBJ/$(echo "$rel" | tr '/' '_').o"
  [ -f "$o" ] && [ "$o" -nt "$TFLM/$rel" ] || g++ $CXXFLAGS $INC -c "$TFLM/$rel" -o "$o"
  OBJS+=("$o")
done < "$HERE/tflm_srcs.txt"

g++ $CXXFLAGS $INC "$HERE/correctness.cpp" "${OBJS[@]}" -o "$HERE/correctness"
"$HERE/correctness"
