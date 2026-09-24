#!/bin/bash
# Regenerates data/ (about 8 minutes: bn2.c's training, all five folds) and everything derived from it. Not needed to run run.sh: the results of
# both steps are committed (data/, models/, bnc_*.h, results.json).
cd "$(dirname "$0")"
BN=${BN_CSV:-../train/bn.csv}                       # UCI Banknote authentication, 1372 rows
# 1. ../train/bn2x.c = ../train/bn2.c plus an export of every fold's quantized rows and of the float perceptron; the training is untouched,
#    so its wave parameters for fold 1 are the committed ../models/banknote/wave_params.h and wave_vectors.h
mkdir -p data && cp "$BN" data/bn.csv && ( cd data && gcc -O3 ../../train/bn2x.c -o bn2x -lm && ./bn2x > bn2x.out && rm -f bn2x bn.csv )
# 2. the models (a virtualenv with numpy scikit-learn emlearn setuptools; on Python 3.12 emlearn needs setuptools for distutils)
python3 -m venv .venv && .venv/bin/pip install numpy scikit-learn emlearn setuptools
.venv/bin/python gen_models.py data
# 3. build, simulate, report
HAPI=${HAPI:-../../../include} ./run.sh
