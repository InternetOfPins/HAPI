# hls_smoke

Proof that HAPI's compile-time composition survives a real high-level-synthesis
(HLS) backend, not just a normal C++ compiler's optimizer. This example
synthesizes to actual hardware (Verilog RTL) through
[PandA-Bambu](https://github.com/ferrandi/PandA-bambu), with the flattened
`Chain<>` inheritance collapsing to a single adder -- proportional to the
real computation, no bloat from the five composed layers.

## What it is

Structurally identical to [`examples/free`](../free) -- same `WrapWith<oc,cc>`
components, same 5-layer `Chain<Parens,SqBracks,Bracks,Bars,XTag>` composition
-- with one change: the wrapped layers add the ordinal value of `oc`/`cc`
instead of printing them with `cout`. That's the only edit needed to make the
composed API synthesizable. I/O calls have no hardware equivalent, so an HLS
backend can't turn `cout<<` into a circuit; arithmetic it can.

`wrapSum(int x)` is the actual synthesis target. `main()` is a host-side
sanity check that prints the result -- it is **not** part of what gets
synthesized, exactly like a testbench driver in any real HLS flow. Unlike
the other `examples/`, this one has no Arduino/embedded build target: its
only job is proving synthesizability, so `native` (for the sanity check) and
Bambu (for the actual HLS run) are the only two things that matter.

Three compile-time `static_assert`s guard the properties this example (and
the zero-overhead claim) rely on: the collapsed `Chain<>`/`APIOf<>` type
stays non-polymorphic (no vtable -- no HLS backend can synthesize one),
`sizeof` doesn't grow past the innermost `Item` (Empty Base Optimization
holding across all 5 layers), and the type stays trivially destructible.
These fail the build immediately, with no Bambu run needed, if a future HAPI
change breaks any of them.

## Run it natively (regression check)

```sh
pio run -e native
.pio/build/native/program   # prints 888
```

`888 = 5 (the argument) + 883 (sum of the ordinal values of the ten wrap
characters '(' ')' '[' ']' '{' '}' '|' '|' '<' '>')`.

## Run it through Bambu HLS

Get the prebuilt AppImage (no Docker, no LLVM/GCC build needed):

```sh
curl -L -o bambu.AppImage https://release.bambuhls.eu/bambu-2024.10.AppImage
chmod +x bambu.AppImage
```

Bambu's frontend compiles internally as a 32-bit (`i386`) target, so it needs
32-bit glibc headers most desktop installs don't have by default:

```sh
sudo apt install gcc-multilib g++-multilib   # or just libc6-dev-i386
```

> Before running that on a machine with a large pending-upgrade backlog,
> dry-run it first with `apt-get install -s gcc-multilib g++-multilib`
> (no root needed) -- on a system that's behind on updates it can pull in
> far more than the two named packages, including a kernel update. Not
> expected on a clean/up-to-date system.

Then synthesize `wrapSum`:

```sh
./bambu.AppImage -I../../include --std=gnu++17 --compiler=I386_CLANG16 \
  --top-fname=wrapSum -v2 src/main.cpp
```

Expect a clean `Summary of resources` ending in:

```
- constant_value: 1
- ui_plus_expr_FU: 1
```

One adder, one folded constant (`10'b1101110011` = 883, the same constant
computed above). Because `x` is a genuine hardware input port, not a literal,
nothing here is constant-folded away before scheduling -- this is the real
generated RTL for the 5-layer `Chain<>` collapse, and it's exactly
proportional to the underlying computation.

## Automated check

`run_hls_smoke.sh` runs the same two checks as a CI-style regression guard:
the `static_assert`s (no Bambu needed), then a real Bambu synthesis with an
adder-count check against Bambu's own `Summary of resources:` output --
**not** a `grep` over the generated Verilog. A single real functional-unit
instance shows up as that substring roughly 5 times in the .v file (module
definition, wire name, instantiation line, parameter continuation, assign
statement), so counting text occurrences is not a valid instance count.

```sh
./run_hls_smoke.sh /path/to/bambu.AppImage /path/to/HAPI/include
```

## Known caveat (Bambu-side, not HAPI-side)

Bambu 2024.10 segfaults in its own internal `FixStructsPassedByValue` IR pass
if an unsynthesizable call (like `cout<<`, `printf`, ...) happens inside a
method reached through class inheritance -- confirmed with minimal non-HAPI
reproducers, so it's a generic Bambu bug, not something specific to HAPI's
template machinery. If you modify this example to add a debug print inside
`WrapWith::Part::api` or `Item::api`, expect a crash rather than the clean
"no functional unit" diagnostic Bambu gives for the same call made directly
in `main()`. Keep I/O out of anything reachable from `--top-fname` and this
doesn't come up -- which is what any real HLS target requires anyway.
