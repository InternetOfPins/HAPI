# Model provenance

`hello_world_float.tflite` and `hello_world_int8.tflite` are copied verbatim
from **tensorflow/tflite-micro**, path
`tensorflow/lite/micro/examples/hello_world/models/`, revision
`f59d98795086b522d7d53f683a0df3ee704afc18` (2026-08).

They are the trained "hello world" sine-approximation network:
`FullyConnected(1->16) + ReLU`, `FullyConnected(16->16) + ReLU`,
`FullyConnected(16->1)`. tflite-micro is Apache-2.0 licensed.

`hello_world_*_model.h` are `xxd -i` C arrays of those files (regenerate with
`tools/get_tflm.sh`). `../hello_world_weights.h` is the same weights extracted
as `constexpr float` arrays for the HAPI net (`tools/extract_weights.py`).
