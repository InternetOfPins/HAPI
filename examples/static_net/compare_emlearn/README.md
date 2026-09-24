# banknote: our cells against emlearn, one task, one build setup

The Banknote task of the running example (`../train/bn2x.c`): UCI Banknote authentication, 4 features quantized to 8 bits with each training fold's min/max, 5-fold CV with the
shuffle of `bn2.c` (seed 42). Every model is trained on each fold's training rows and scored on that fold's held-out rows, so all accuracies are
on the same folds, and the device programs run fold 1's 274 held-out rows (`../models/banknote/wave_vectors.h`).

| model | what it is |
|---|---|
| `wave4` | the masked-wave cell of `bn2.c` (`../models/banknote/net.h`, `wave_params.h`): shifts, masks and adds |
| `lin4`, `table4` | the float perceptron of `bn2.c`, quantized (int8 weights, int32 accumulator, int16 product); `table4` is the same weights as a PROGMEM loop |
| `eml_tree_dN_f32`, `_u8` | emlearn 0.23.2 inline decision tree of depth N (sklearn `DecisionTreeClassifier`), features as `float` (emlearn's default) and as `uint8_t` (its `dtype` option) |
| `eml_mlp_hN` | emlearn `eml_net`, MLP 4-N-1 (sklearn `MLPClassifier`, relu, best of 5 restarts on the training rows), float; the 1/255 input scaling is folded into layer 0 |
| `null` | no model: the harness, the row fetch and the call. The `net` columns subtract it |

`./run.sh` builds every program with one set of flags (avr-gcc 7.3, `-Os -ffunction-sections -fdata-sections -Wl,--gc-sections`), runs each on simavr
(ATmega328p, 16 MHz) through one harness (`bnc_common.h`), and prints `results.md`: flash and RAM, how many of the 274 rows the *device* got right, and
the min/mean/max cycles over the rows, next to the accuracy on all five folds. So every size/cycle pair comes from the same build, and the on-device
correct count equals the host's accuracy for every model (a check that the same function ran). `regen.sh` reproduces `data/` (8 minutes) and everything
derived from it; the generated files are committed, and regenerating from `data/` reproduces them byte for byte.

## Read with care
* **Cycles come from simavr**; `../measure/silicon.py` ran the same ELFs on a real ATmega328p (an Arduino Nano) and every number was identical (`../measure/silicon_results.md`).
* Banknote is nearly separable; this says nothing about a harder task (Sonar is the harder one).
* The trees are as emlearn generates them (its majority-vote wrapper included); the MLP as emlearn's `eml_net` (float, `expf`, from libm) with `static const`
  weights, which avr-gcc copies to RAM (the RAM column). emlearn 0.23.2 has no working fixed-point MLP (`eml_net_forward_q16` is a stub with `FIXME: implement`, its
  test is `xfail`) and its MLP `inline` method silently emits the loadable code, so the MLP rows are the loadable float `eml_net`, its only working MLP; see `emlearn_repro.py`
  (a reproduction, checked against master) and `emlearn_note.md` (a draft message to the author, not sent).
* Model choices (depths, hidden sizes) span the range rather than picking one: the smallest emlearn model here (depth 2, uint8) is already larger than
  our cells, and trees need depth 8 to reach the accuracy of `lin4`/`wave4`.
* No AOT tool on the Cortex-M and no other library was compared.
