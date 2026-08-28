#!/usr/bin/env bash
# Generate lib/tflm/ -- the minimal TFLite-Micro source tree this example
# builds against. Needs git, make, and network (the TFLM Makefile downloads
# flatbuffers/gemmlowp/ruy/kissfft on first run). Does NOT need TensorFlow.
#
# lib/tflm/ is gitignored (it is ~5 MB of third-party code); this script,
# plus the models and hello_world_weights.h that ARE checked in, is what
# makes the example reproducible -- the same way hls_fir needs Bambu
# installed separately.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
EX="$(dirname "$HERE")"
PIN=f59d98795086b522d7d53f683a0df3ee704afc18   # tensorflow/tflite-micro, 2026-08

WORK="$(mktemp -d)"; trap 'rm -rf "$WORK"' EXIT
git clone --quiet https://github.com/tensorflow/tflite-micro.git "$WORK/tm"
git -C "$WORK/tm" checkout --quiet "$PIN"

cd "$WORK/tm"
# The TFLM Makefile pulls in every examples/*/Makefile.inc at parse time,
# some of which run codegen needing numpy/Pillow. We only want hello_world,
# so drop the rest before generating.
find tensorflow/lite/micro/examples -name Makefile.inc ! -path '*hello_world*' -delete
find signal -name Makefile.inc -delete 2>/dev/null || true

rm -rf "$EX/lib/tflm"
# create_tflm_tree.py drives the TFLM Makefile with relative paths -- run
# from the repo root. Prefer the system python3 (has what the Makefile's
# remaining scripts need) over a stripped PlatformIO python that may be
# first on PATH.
PY=python3; [ -x /usr/bin/python3 ] && PY=/usr/bin/python3
"$PY" tensorflow/lite/micro/tools/project_generation/create_tflm_tree.py \
  -e hello_world --rename_cc_to_cpp "$EX/lib/tflm"

# library.json: scope the compiled sources to the 37 files this example needs
python3 - "$EX" <<'PY'
import json, os, sys
EX = sys.argv[1]
srcs = [l.strip() for l in open(os.path.join(EX, "check/tflm_srcs.txt")) if l.strip()]
lib = {"name": "tflm", "version": "0.0.0+hello_world",
       "build": {"flags": ["-I.", "-Ithird_party/flatbuffers/include",
                            "-Ithird_party/gemmlowp", "-Ithird_party/ruy",
                            "-DTF_LITE_STATIC_MEMORY", "-DTF_LITE_DISABLE_X86_NEON"],
                 "srcFilter": ["-<tensorflow/>", "-<signal/>"] + [f"+<{s}>" for s in srcs],
                 "libArchive": False}}
open(os.path.join(EX, "lib/tflm/library.json"), "w").write(json.dumps(lib, indent=2) + "\n")
PY

echo "lib/tflm ready ($(du -sh "$EX/lib/tflm" | cut -f1)) -- pinned to tflite-micro $PIN"
echo "the checked-in models/ + hello_world_weights.h already match this revision"
echo "(models/PROVENANCE.md has the regeneration steps)."
