# hls_can_disabler

Checks whether HAPI's `Chain<>`/`APIOf<>` composition reads naturally for
**access-control logic**, not just DSP taps (`hls_fir`) or format-string
parsing (OneParse's `hls_smoke`) — a compile-time whitelist of allowed
(mailbox, CAN-ID) pairs, gated behind a stateful minimum-retransmission-
interval guard, synthesized through a real HLS backend to get an actual
footprint number.

**Scope note:** this example is an original design built to exercise the
composition model on this problem shape, *not* a reproduction of any
specific published CAN-bus-disabler paper. An earlier session referenced
such a paper (mailbox/ID spoof detection, a minimum-cycle replay check,
reported LE counts on an Altera baseline) but its actual text was never
available in this session to reproduce byte-for-byte or to cite numbers
against — see [Not done](#not-done) below. Nothing here should be read as
a verification or refutation of that paper's own results.

## The design

- **`Allow<Mailbox,Id>`** — one whitelist entry: a stateless `Chain<>`
  layer (same "pure predicate per layer" shape as `hls_smoke`'s
  `WrapWith<>`) that permits transmission if the caller's `(mailbox,id)`
  matches, and defers to the next layer otherwise. `Deny` closes the
  chain: ran out of entries, not permitted.
- **`MinCycle<MinInterval>`** — one stateful outer layer (same "a layer
  can own real state" idea `hls_fir`'s `Tap<>` demonstrates): tracks the
  tick of the last *permitted* transmission and a `armed` flag (so the
  very first transmission isn't blocked for lack of history), and only
  reports permit when the whitelist agrees *and* enough ticks have
  passed since the last permit — a flood/replay guard.
- **`DisablerTop`** (`APIOf<Deny, Chain<MinCycle<100>,
  Allow<0,0x100>,Allow<1,0x101>,Allow<2,0x102>>>`) — the closed gate.
  `permit(mailbox,id,tick) -> bool`.

`tick` is a plain function argument here, not a live hardware counter —
see [Not done](#not-done) for why that matters before citing this
against any bus-timing-driven design.

## Run it natively (regression check, all five cases)

```sh
pio run -e native
.pio/build/native/program
```

```
legit transmit               got=true  expect=true  PASS
mailbox-3 spoof              got=false expect=false PASS
id-0x200 spoof               got=false expect=false PASS
in-cycle retransmit          got=false expect=false PASS
past-cycle retransmit        got=true  expect=true  PASS

ALL PASS
```

- **legit transmit** — mailbox 0, id `0x100` is whitelisted, first call
  (not armed yet) — cycle check is skipped, permitted.
- **mailbox-3 spoof** — id `0x100` *is* whitelisted, but not for
  mailbox 3 — `Allow<>` checks the pair, not the id alone. Blocked.
- **id-0x200 spoof** — no mailbox is whitelisted for id `0x200`.
  Blocked.
- **in-cycle retransmit** — the legit pair retransmitted 50 ticks after
  the first permit, below the 100-tick minimum interval. Blocked.
- **past-cycle retransmit** — the legit pair retransmitted 150 ticks
  after the first permit, past the minimum interval. Permitted.

## Run it through Bambu HLS

Get the prebuilt AppImage (no Docker, no LLVM/GCC build needed):

```sh
curl -L -o bambu.AppImage https://release.bambuhls.eu/bambu-2024.10.AppImage
chmod +x bambu.AppImage
export BAMBU_APPIMAGE="$(pwd)/bambu.AppImage"
```

Bambu's frontend compiles internally as a 32-bit (`i386`) target:

```sh
sudo apt install gcc-multilib g++-multilib   # or just libc6-dev-i386
```

Then:

```sh
pio run -e hls -t synthesize-can-disabler
```

RTL and Bambu's logs land in `.hls_out_can_disabler/` (gitignored).

## Results (verified, not estimated)

Against `xc7a100t-1csg324-VVD` (Xilinx Artix-7, Digilent Arty A7/Nexys
A7 — widely owned, not a special-order part), `--clock-period=10`
(100MHz target) — same device/period as `hls_fir` and OneParse's
`hls_smoke` for rough comparability:

| Metric | `canDisablerTop` |
|---|---|
| Flip-flops | **22** |
| Registers | 3 (SE:1 + STD:2) |
| Distributed-RAM elements (`ARRAY_1D_STD_DISTRAM_NN_SDS`) | 2 |
| `mult_expr_FU` | **0** |
| **Estimated number of DSPs** | **0** |
| Modules instantiated | 27 |
| Control steps | 6 |
| States | 4 |
| Cycles (min/max) | 2 / 3 |
| Estimated max frequency | 151.14 MHz |
| Minimum slack | 3.384 ns |
| Estimated area (logic, no muxes) | 2079 |
| Estimated area (MUX21) | 68 |
| **Total estimated area** | **2147** |

No `mult_expr_FU`/DSP usage at all — every operation in this design
(equality checks, one subtraction, one comparison) is plain
combinational/comparison logic, so there was never a multiply for Bambu
to strength-reduce or map to a DSP block in the first place — a
different, simpler resource story than `hls_fir`'s tap networks. Total
area (2147) is roughly a third of `hls_fir`'s smallest target
(`fir4Top`, 7590) — consistent with this being control/comparison logic
rather than an arithmetic datapath.

`Total number of flip-flops` (22) is noticeably larger than the raw
`sizeof(DisablerTop) == 8` (one `uint32_t` + one `bool`, confirmed by
the `static_assert` in `src/main.cpp`) would suggest — the 8-byte state
is the *architectural* register cost; Bambu's actual flip-flop count
also covers pipeline/control-step bookkeeping (state register, done
signal, intermediate comparison results held across the 2–3 cycle
schedule) that has no footprint in `sizeof()` at all. Same lesson
`hls_fir`'s README draws about flip-flop count vs. `ARRAY_1D_STD_DISTRAM`
count: no single struct-level metric predicts the synthesized register
cost by itself.

## Not done

- **No paper comparison.** The original handoff this example continues
  cited a specific paper's LE counts (3106 LE added on a 5374-LE
  baseline, 23,888-LE full system, on an Altera device family) as a
  target to compare against. That paper's actual text/methodology was
  not available in this session — comparing this design's 2147
  Artix-7-characterized area units against an unread paper's
  Altera-characterized LE count would be citing a number nobody here has
  verified means the same thing on the same technology. If/when the
  paper is available, this section should be filled in with an explicit
  device-family caveat, not a bare number-vs-number claim.
- **The `tick`-argument question, still open.** `MinCycle<>` compares
  two `uint32_t` values passed in by the caller; a real CAN-bus
  minimum-cycle-time enforcement would tie that comparison to a genuine
  hardware counter clocked off bus timing (the kind of few-microsecond,
  few-bit measurement a real bus-timing paper would specify). Whether
  "pass tick as a function argument" is a fair functional stand-in for
  that, or needs a real free-running counter wired into `canDisablerTop`
  before any latency/timing claim is citable, is unresolved here for the
  same reason as the paper comparison above: no source to check the
  claim against.
- **Real P&R** (Vivado / Yosys+nextpnr-ecp5) — Bambu's area/DSP numbers
  above are pre-placement, not confirmed post-P&R LUT/FF/DSP-slice
  mapping.
- **RTL simulation** for cycle-accurate throughput — the control-step/
  state counts above are Bambu's static schedule, not a
  testbench-driven measurement.
- **Only 3 whitelist entries.** A real ECU's mailbox table would be
  larger; this example doesn't test how area scales with whitelist
  size (an easy follow-up: add `Allow<>` layers and re-synthesize, the
  same resource-scaling question `hls_fir` answers for tap count).
