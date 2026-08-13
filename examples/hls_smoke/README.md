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

## Target device

Bambu is target-aware, not target-independent: functional-unit selection
and every area/frequency/slack number are characterized against a
specific device technology library -- a run with no `--device-name`
produces numbers against Bambu's undocumented internal default, which
aren't citable against any real, ownable board. **This example originally
ran against that undocumented default** (the raw invocation this section
used to show had no `--device-name`/`--clock-period` at all); it now pins
the same device/period as `hls_fir`/`hls_can_disabler`:

```
--device-name=xc7a100t-1csg324-VVD --clock-period=10
```

`xc7a100t-1csg324-VVD` is the Xilinx Artix-7 on the Digilent Arty A7/
Nexys A7 (10ns targets 100MHz). The structural result (exactly one real
`ui_plus_expr_FU`, zero flip-flops, zero DSPs) is unchanged from the
original default-device run — only the frequency/slack/area figures
below are new, now citable against a confirmed device instead of an
undocumented one.

## Run it through Bambu HLS

Get the prebuilt AppImage (no Docker, no LLVM/GCC build needed):

```sh
curl -L -o bambu.AppImage https://release.bambuhls.eu/bambu-2024.10.AppImage
chmod +x bambu.AppImage
export BAMBU_APPIMAGE="$(pwd)/bambu.AppImage"
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

Then synthesize `wrapSum` as a real PlatformIO custom target (wired via
`extra_hls.py`, `env.AddCustomTarget` -- same pattern as `hls_fir`/
`hls_can_disabler`):

```sh
pio run -e hls -t synthesize-wrapsum
pio run -e hls -t synthesize-wrapsum-gcc8       # GCC8 frontend cross-check (fails, see Results)
pio run -e hls -t synthesize-wrapsum-altdevice  # Lattice ECP5 cross-check
```

Without `BAMBU_APPIMAGE` set, all three fail immediately with a clear
message naming the missing prerequisite. RTL and Bambu's logs land in
`.hls_out_wrapsum/`, `.hls_out_wrapsum_gcc8/`, `.hls_out_wrapsum_altdevice/`
respectively (gitignored).

## Results (verified, not estimated)

| Metric | Bambu / clang16, Artix-7 (primary) | Bambu / GCC8, Artix-7 | Bambu / clang16, Lattice ECP5 |
|---|---|---|---|
| Flip-flops | **0** | N/A — see below | **0** |
| `ui_plus_expr_FU` | **1** | N/A | **1** |
| **Estimated number of DSPs** | **0** | N/A | **0** |
| Modules instantiated | 4 | N/A | 4 |
| Control steps | 3 | N/A | 3 |
| States | 1 | N/A | 1 |
| Cycles (min/max) | 1 / 1 | N/A | 1 / 1 |
| Estimated max frequency | 388.50 MHz | N/A | 225.23 MHz |
| Minimum slack | 7.426 ns | N/A | 5.560 ns |
| **Total estimated area** | **18** | N/A | **33** |

One real adder, one folded constant (`10'b1101110011` = 883, the same
constant computed above), zero flip-flops — the entire 5-layer `Chain<>`
collapse is purely combinational (1 state, 1 cycle): nothing here needs to
persist across a clock edge, so there's no register cost at all, unlike
`hls_fir`'s taps or `hls_can_disabler`'s cycle-gated state. Because `x` is
a genuine hardware input port, not a literal, nothing is constant-folded
away before scheduling — this is the real generated RTL for the 5-layer
collapse, exactly proportional to the underlying computation (one add per
layer, folded into a single functional unit by Bambu's own scheduling).

### Cross-tool/cross-config validation

Bambu is currently the only HLS backend actually run against this design
— a Bambu-specific quirk could in principle masquerade as a HAPI property
(or vice versa). Two independent Bambu configs were run as a first
cross-check; further independent tools are tracked the same way as
`hls_fir`/`hls_can_disabler`:

| Tool | Status |
|---|---|
| Bambu (clang16 frontend) | **Done** — primary Results table above |
| Bambu (GCC8 frontend) | **Done** — rejects HAPI, see below |
| Bambu (Lattice ECP5 device) | **Done** — see below |
| Vitis HLS | **Not run** — integration scaffolding ready (`extra_hls_vitis.py`, `vitis/run_hls.tcl`, `[env:hls-vitis]`); blocked on Xilinx account + Vitis Unified Installer, an interactive step not done in this pass |
| Intel HLS Compiler | **Ruled out** — the classic `i++` command-line compiler this pass targeted now appears to require Quartus Prime **Pro** edition (paid); the free Lite-edition add-on catalog only offers a different, newer tool ("HLS IP Gen (Beta)") with a different IP-generation workflow, not a drop-in. No integration script kept |
| LegUp | **Ruled out** — see `HAPI/.RnD/legupHLS/HANDOFF.md` (frozen pre-C++17 academic snapshot, closed commercial successor) |

- **GCC8 frontend: rejected, not a HAPI bug.** Fails at parse time —
  `Unrecognized keyword ... bound_template_template_parm` / `Parse error`
  — identical to the same failure on `hls_fir`/`hls_can_disabler`. Bambu's
  GCC8-based tree parser doesn't recognize the AST node for a
  template-template-parameter binding that `Chain<>`'s recursive
  composition produces; `I386_CLANG16` remains the only viable frontend
  for this codebase. Full writeup: `HAPI/.RnD/bambuHLS/HANDOFF.md`
  finding #4.
- **Lattice ECP5 device: structural result is device-independent.** One
  `ui_plus_expr_FU`, zero flip-flops, zero DSPs, 1 control step/cycle —
  identical across both devices. Only the continuously-valued figures
  (area 18 → 33, frequency 388.50 → 225.23 MHz, slack 7.426 → 5.560 ns)
  differ, as expected from each device's own technology library.

## Automated check

`run_hls_smoke.sh` runs the same two checks as a CI-style regression guard:
the `static_assert`s (no Bambu needed), then a real Bambu synthesis (now
pinned to the same explicit device/period as the `pio run -e hls` targets
above) with an adder-count check against Bambu's own
`Summary of resources:` output -- **not** a `grep` over the generated
Verilog. A single real functional-unit instance shows up as that substring
roughly 5 times in the .v file (module definition, wire name, instantiation
line, parameter continuation, assign statement), so counting text
occurrences is not a valid instance count.

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
