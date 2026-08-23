# contractCompose

Runtime Design-by-Contract (precondition gating, optional postcondition)
expressed through HAPI's existing `Chain<>`/`Part<>` composition axis, not
a new orthogonal mechanism. Not proof-transport -- nothing is statically
discharged; this is an ordinary runtime check placed at the seam where a
component's real behavior gets invoked, opt-in per composition.

Two independent, valid variants, each guarding a different boundary.

## `strict_broker.cpp` -- named per component, hard error

A `contract::` namespace of free functions, one overload per checked
component's `Part<O>`. `StrictBroker` names exactly the methods it wants
checked and calls into `contract::` for those; a missing overload for a
named method is a **hard compile error**, by design (see
`hard_error_demo.cpp`). Untouched methods pass straight through via
ordinary inheritance, zero cost, no mention required.

```sh
pio run -e strict_broker
.pio/build/strict_broker/program
```

## `tail_contract.cpp` -- hosted in the broker, tail-injected, soft fallback

The checks (`Broker::pre_f`/`post_f`) and the adapter that applies them
(`Broker::Contract`) both live inside one `Broker`, injected as the
**last** element of the composition list. Per `Chain<O,OO...>::Part<T> =
O::Part<Chain<OO...>::Part<T>>`, last-in-list is innermost, so `Contract`
wraps the real terminal API directly -- every call that reaches all the
way down passes through it exactly once, regardless of which named
component up the chain triggered it. Because of that, a missing check
here means "no check for this method," not "forgot to wire something,"
so this variant uses an SFINAE soft fallback instead of a hard error.

```sh
pio run -e tail_contract
.pio/build/tail_contract/program
```

## `hard_error_demo.cpp` -- deliberately fails to compile

Documents `strict_broker.cpp`'s hard-error property standalone, including
the exact expected diagnostic.

```sh
pio run -e hard_error   # EXPECTED to fail -- that's the point
```

## Both variants demonstrate

- A check may reach past its own call's argument into whatever else
  `Base` exposes (here, a live `budget`/`remaining()` on `RawApi`) -- the
  same liberty an ordinary composed `Part` already has when it calls
  `Base::f(x)`, not just a pure function of that call's own
  argument/return value.
- The gate is entirely `check()`'s contract (must never return normally
  on failure), not code ordering.
- Same component, checked and unchecked builds, zero edits to the
  component -- decided once, at composition, by whether the broker is in
  the list. Same broker, reused verbatim across unrelated components.

## Scope

Host-only (`g++`/native), verified against local `HAPI` `main`. Not yet
checked on avr-gcc, arm-none-eabi-gcc, RISC-V, or Bambu/Vitis HLS front
ends -- particularly worth re-checking whether the hard-error property
fires at the same point (composing the type) on every toolchain; observed
on GCC specifically, not assumed universal.
