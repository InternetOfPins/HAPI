# Banknote wave4 on Compiler Explorer

There are two ready-to-paste sources for [Compiler Explorer](https://godbolt.org). Both hold the same trained Banknote cell from `examples/static_net`:

| file | `waveCell.h` in it |
|---|---|
| `wave4_od.cpp` | written with Open Derivation (`:`) and translated by `../translate.py`. The `:` source (`../round2/src/waveCell.h`) is the leading comment |
| `wave4_apiof.cpp` | the original, hand-written HAPI form (`Cell = hapi::APIOf<API,OO...,Bias<k>>`) |

Each is self-contained except for one line, which includes HAPI's single header by raw GitHub URL, pinned to the commit that added it:

```c++
#include <https://raw.githubusercontent.com/InternetOfPins/HAPI/3b0c466ca5a6870a0747abc8133fd64743c333b7/single/hapi.h>
```

Everything else is inlined, unchanged: static_net's `staticNet.h`, `waveCell.h` (in one of its two forms), `wave_params.h` and `net.h`, and
`wave4()`, which is `compare_emlearn/bnc_wave.h`'s `bnc_predict` under another name.

## Settings

| | |
|---|---|
| language | C++ |
| compiler | **AVR gcc 7.3.0**: the toolchain every static_net figure was measured with (Ubuntu's `gcc-avr`, 7.3.0). If Compiler Explorer's list has no 7.3.0, take the nearest AVR gcc it offers. Other versions should compile it (C++17), but their instruction counts are not the ones below |
| flags | `-std=c++17 -Os -mmcu=atmega328p` |

To compare the two, open one source pane per file, each with its own compiler pane (same compiler and flags), or use Compiler Explorer's diff view.

## What to expect

`verify.sh` checks this with avr-gcc 7.3.0, against the exact bytes the URL serves (`verify.log`):

- **Identical:** both sources give the same `wave4(unsigned char const*)`, 28 instructions, 56 B.
- **The same as static_net's build:** that `wave4()` is instruction for instruction the `bnc_predict` static_net builds from `include/` for `compare_emlearn`.
- **The 44 B figure:** it is exactly this function minus the null model's `bnc_predict` (12 B, a single compare), the floor `compare_emlearn/run.sh` subtracts. 56 − 12 = 44.

On Compiler Explorer: <https://godbolt.org/z/TvM334frd>. AVR gcc 7.3.0, -std=c++17 -Os -mmcu=atmega328p.
Source #1 = hand-written hapi::APIOf cell, source #2 = Open Derivation, translated. Diff view empty: wave4() is 28 instructions in both.

The pinned URL was fetched and matches `single/hapi.h` at that commit byte for byte.

## Regenerate

```sh
python3 make.py     # rebuild both sources from the current files (translated output, static_net headers)
./verify.sh         # compile both as Compiler Explorer would, compare wave4() with each other and with static_net's bnc_predict
```

`make.py` pins `single/hapi.h` to `HAPI_SHA`. Bump it when `include/hapi/` changes.
