# train: how the trained constants came about

Training happens on a PC and produces constants; the net is then generated as a type (`models/`). Nothing here runs on the device.

| file | what |
|---|---|
| `bn2x.c`, `bn.csv` | Banknote: 5-fold CV (shuffle seed 42), features quantized to 8 bits with each training fold's min/max; per fold a frequency-only wave, a masked wave (coordinate descent with restarts, seeded from the frequency-only one) and a float pocket perceptron. Writes `wave_params.h` and `wave_vectors.h` (fold 1: the cell of the running example and its 274 held-out rows) and, per fold, `fold<N>.csv` (quantized train and test rows) and `perceptron.csv` (used by `compare_emlearn/`). About 8 minutes: `gcc -O3 bn2x.c -o bn2x -lm && ./bn2x` |
| `bn2.c` | the same trainer without the export |
| `sonar_lin_train.c`, `gen_lin_sonar.py`, `sonar.csv` | Sonar: the float pocket perceptron per fold, then its weights quantized to int8 (the bias scaled by 255, because the float model sees `x/255` and the integer one the raw byte) and written as `lin::CellOf<...>` types, narrow and wide, plus the held-out rows: `gcc -O2 sonar_lin_train.c -o t -lm && ./t > sonar_lin_train.out && python3 gen_lin_sonar.py` writes `sonar_lin_params.h` and `sonar_lin_vectors.h` (identical to `models/sonar/`) |

`bn2x.c` reproduces the committed `models/banknote/wave_params.h` and `wave_vectors.h` exactly, and the Sonar pipeline reproduces `models/sonar/` exactly.

Data, unmodified copies from the UCI Machine Learning Repository, cite them if you reuse them:
* `bn.csv`: *Banknote Authentication* (Volker Lohweg, 2012), 1372 rows, 4 features from wavelet transforms of banknote images, https://archive.ics.uci.edu/dataset/267/banknote+authentication
* `sonar.csv`: *Connectionist Bench (Sonar, Mines vs. Rocks)* (R. Paul Gorman and Terrence J. Sejnowski, 1988), 208 rows, 60 features, https://archive.ics.uci.edu/dataset/151/connectionist+bench+sonar+mines+vs+rocks
