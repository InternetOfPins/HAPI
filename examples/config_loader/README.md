# config_loader

A small CLI config loader/validator combining all three libraries in an
ordinary, non-embedded context: **OneParse** parses `key = value` lines
structurally (it has no notion of which keys are "expected" — that's a
separate, semantic concern), a **HAPI** `Chain<>` of per-field validator
layers does the required/range checks, and each layer owns its state as
**OneData** `Data<T>`/`Watch<Int>` — required/range validation plus
change-tracking across a reload.

This is the family's first example with **no embedded target at all** —
`platformio.ini` has only `[env:native]`, no board section, matching
`OneParse/examples/{basic,kelvin,ebnf,json}`'s own established convention
for examples that never target hardware.

## Run

```sh
pio run -e native
.pio/build/native/program data/config.txt data/config_v2.txt
```

Loads `data/config.txt` (all four fields valid), then reloads
`data/config_v2.txt` (port changed, `timeout` missing, `retries` out of
range) and reports what changed since the first load.

## Known limitation

Unknown/extra keys in the config file are silently ignored — the grammar
only recognizes "identifier = token" structurally; only the validator layer
knows which keys are expected. A reasonable minimal-scope choice for a
demonstrator, not a hidden gap.

`name` is a plain `Data<char[32]>` with no `Watch<>` wrapper, deliberately:
`Watch<W>::changed()` compares `get() != watched`, which for an array type
compares pointer/decay values, not string contents — a real composability
edge case, not something to paper over.
