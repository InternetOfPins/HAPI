# hls_can_disabler

Checks whether HAPI's `Chain<>`/`APIOf<>` composition reads naturally for
**access-control logic**, not just DSP taps (`hls_fir`) or format-string
parsing (OneParse's `hls_smoke`) — a compile-time whitelist of allowed
(mailbox, CAN-ID) pairs, gated behind a stateful minimum-retransmission-
interval guard, synthesized through a real HLS backend to get an actual
footprint number.

**Scope note:** this example is an original design built to exercise the
composition model on this problem shape, *not* a reproduction of any
specific published CAN-bus-disabler paper — it was designed and Bambu-
verified before the paper below was actually read. See
[Comparison to the CAN Disabler paper](#comparison-to-the-can-disabler-paper)
for the structural differences found once it was, and for why no
number-vs-number footprint comparison is offered.

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
see [Comparison to the CAN Disabler paper](#comparison-to-the-can-disabler-paper)
for why that matters before citing this against any bus-timing-driven
design.

`Deny`/`Allow<>`'s `allowed()` is `constexpr noexcept`, which buys a
compile-time regression check on the whitelist logic itself
(`static_assert(DisablerTop::allowed(0,0x100) && ...)` in
`src/main.cpp`, only possible because it's a constant expression) — it
does **not** change the synthesized hardware, confirmed by re-running
Bambu after adding it: identical 22 flip-flops / 0 DSPs / 2147 area to
before, since every real `permit()` call passes runtime `mailbox`/`id`
values, and a trivial one-line static predicate inlines the same way
with or without the keyword.

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
pio run -e hls -t synthesize-can-disabler-gcc8       # GCC8 frontend cross-check (fails, see Results)
pio run -e hls -t synthesize-can-disabler-altdevice  # Lattice ECP5 cross-check
```

RTL and Bambu's logs land in `.hls_out_can_disabler/`,
`.hls_out_can_disabler_gcc8/`, `.hls_out_can_disabler_altdevice/`
respectively (gitignored).

## Results (verified, not estimated)

Against `xc7a100t-1csg324-VVD` (Xilinx Artix-7, Digilent Arty A7/Nexys
A7 — widely owned, not a special-order part), `--clock-period=10`
(100MHz target) — same device/period as `hls_fir` and OneParse's
`hls_smoke` for rough comparability:

| Metric | Bambu / clang16, Artix-7 (primary) | Bambu / GCC8, Artix-7 | Bambu / clang16, Lattice ECP5 |
|---|---|---|---|
| Flip-flops | **22** | N/A — see below | **57** |
| Registers | 3 (SE:1 + STD:2) | N/A | 7 (SE:3 + STD:4) |
| Distributed-RAM elements (`ARRAY_1D_STD_DISTRAM_NN_SDS`) | 2 | N/A | 2 |
| `mult_expr_FU` | **0** | N/A | **0** |
| **Estimated number of DSPs** | **0** | N/A | **0** |
| Modules instantiated | 27 | N/A | 30 |
| Control steps | 6 | N/A | 7 |
| States | 4 | N/A | 5 |
| Cycles (min/max) | 2 / 3 | N/A | 3 / 4 |
| Estimated max frequency | 151.14 MHz | N/A | 160.80 MHz |
| Minimum slack | 3.384 ns | N/A | 3.781 ns |
| Estimated area (logic, no muxes) | 2079 | N/A | 2130 |
| Estimated area (MUX21) | 68 | N/A | 98 |
| **Total estimated area** | **2147** | N/A | **2228** |

No `mult_expr_FU`/DSP usage at all — every operation in this design
(equality checks, one subtraction, one comparison) is plain
combinational/comparison logic, so there was never a multiply for Bambu
to strength-reduce or map to a DSP block in the first place — a
different, simpler resource story than `hls_fir`'s tap networks. Total
area (2147) is roughly a third of `hls_fir`'s smallest target
(`fir4Top`, 7590) — consistent with this being control/comparison logic
rather than an arithmetic datapath.

### Cross-tool/cross-config validation

Bambu is currently the only HLS backend actually run against this design —
a Bambu-specific quirk could in principle masquerade as a HAPI property (or
vice versa). Two independent Bambu configs were run as a first cross-check
(see `extra_hls.py`'s `-gcc8`/`-altdevice` targets). Two more independent
tools were investigated:

| Tool | Status |
|---|---|
| Bambu (clang16 frontend) | **Done** — primary Results table above |
| Bambu (GCC8 frontend) | **Done** — rejects HAPI's template-template-parameter usage, see below |
| Bambu (Lattice ECP5 device) | **Done** — see below |
| Vitis HLS | **Not run** — integration scaffolding ready (`extra_hls_vitis.py`, `vitis/run_hls.tcl`, `[env:hls-vitis]`); blocked on Xilinx account + Vitis Unified Installer, an interactive step not done in this pass |
| Intel HLS Compiler | **Ruled out** — the classic `i++` command-line compiler this pass targeted now appears to require Quartus Prime **Pro** edition (paid); the free Lite-edition add-on catalog only offers a different, newer tool ("HLS IP Gen (Beta)") with a different IP-generation workflow, not a drop-in. No integration script kept — see `HAPI/.RnD/legupHLS/HANDOFF.md`'s LegUp entry for the same kind of "ruled out, not a compile failure" finding |
| LegUp | **Ruled out** — free academic version is a frozen, single-commit, pre-C++17, VM-only 2015-era snapshot under a non-commercial license; actively-maintained descendant (SmartHLS) is closed/commercial. Full reasoning: `HAPI/.RnD/legupHLS/HANDOFF.md` |

To run the Vitis HLS target once installed:

```sh
export VITIS_HLS=/path/to/vitis_hls   # after Xilinx account + Vitis HLS install
pio run -e hls-vitis -t synthesize-can-disabler-vitis
```

- **GCC8 frontend (`--compiler=I386_GCC8`): rejected, not a HAPI bug.**
  Fails at parse time — `Unrecognized keyword ... bound_template_template_parm`
  / `Parse error` — before scheduling is ever reached. Bambu's GCC8-based
  tree parser doesn't recognize the AST node for a template-template-
  parameter binding that `Chain<>`'s recursive composition produces; the
  bundled clang16 frontend accepts the same construct cleanly. `I386_CLANG16`
  is the only viable frontend for this codebase — see
  `HAPI/.RnD/bambuHLS/HANDOFF.md` finding #4 for the full writeup.
- **Lattice ECP5 device (`LFE5U85F8BG756C`): DSP inference is
  device-independent, flip-flop/area counts are not.** DSP count (0),
  `mult_expr_FU` count (0), and the "no multiply in this design" structural
  finding matched exactly against a second, non-Xilinx vendor's device — the
  strongest form of evidence that this is a real property of the design, not
  a Bambu/Artix-7 artifact. Flip-flop count (22 → 57) and total area
  (2147 → 2228) did **not** match — expected, since each device's technology
  library binds the same logical registers differently; these numbers were
  never claimed to be portable across devices (see original device-choice
  rationale above), and this run confirms that boundary rather than
  overturning it.

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

## Comparison to the CAN Disabler paper

R. Kurachi, T. D. Pyun, S. Honda, H. Takada, H. Ueda, S. Horihata,
**"CAN Disabler: Hardware-based Prevention method of Unauthorized
Transmission in CAN and CAN-FD networks,"** arXiv:2608.03567v1.

### No footprint number is compared, on purpose

The paper reports (Sec. V-B): the full embedded system (NiosII soft
core + DRAM controller + on-chip RAM + redesigned CAN controller)
consumes **23,888 logic elements**; "the CAN controller itself consumes
**5,374** logic elements"; and "there are **3,106** more logic elements
compared with the synthesis using the conventional CAN controller." That
last sentence is the disabler's overhead, but the paper's own wording
leaves it ambiguous whether the 5,374 figure already includes that
3,106 or is the pre-disabler baseline — not resolved here either, quoted
as-is rather than resolved by guessing.

None of those three numbers are commensurable with this example's 2147
Bambu "Total estimated area":

- **Different tool.** The paper's numbers come from Altera's own
  synthesis flow (Quartus, implied by "logic elements" as the unit and
  the DE0-NANO target) on real placed/synthesized hardware. This
  example's numbers come from Bambu's own internal area-estimation
  score, computed pre-placement — not a LUT/LE count from any vendor
  tool at all (see [Not done](#not-done)'s P&R item).
- **Different device family.** Altera Cyclone IV (DE0-NANO) vs. Xilinx
  Artix-7 (`xc7a100t-1csg324-VVD`) — different LUT architectures, so
  even genuine LE-vs-LUT counts wouldn't convert by a fixed ratio.
- **Different scope.** The paper's 3,106/5,374/23,888 are for a full
  CAN-FD controller IP with the disabler embedded as a sub-module inside
  it (register file, protocol processing, bit-timing logic, counter
  process, all in one synthesis run); `canDisablerTop` here is only the
  gate logic in isolation, with no surrounding controller.

Given all three, a "2147 vs. 3106" sentence would imply a precision this
comparison cannot support — so this README states the paper's numbers
for the record and stops there, rather than manufacture a ratio.

### Structural differences found, now that the paper's been read

- **Protection functions 1 and 2 are one combined check here, two
  independent ones in the paper.** The paper's device disabler runs
  three *separate* evaluations in sequence (Fig. 3): (1) is this
  mailbox enabled at all (a per-mailbox enable bit, Table I's "Disable
  mailbox information" register), independent of which CAN-ID it's
  carrying; then (2) is this CAN-ID on the whitelist, independent of
  which mailbox sent it. This example's `Allow<Mailbox,Id>` instead
  whitelists the *pair* directly — a stricter, less configurable policy
  than the paper's two-independent-tables design (there is no way here
  to say "mailbox 0 is enabled for any whitelisted ID" the way the
  paper's Table I schema allows). The `mailbox-3 spoof` and `id-0x200
  spoof` test names read as if they map onto the paper's Evaluation 1
  and Evaluation 2 separately; they don't — both are actually testing
  the one combined `Allow<>` check.
- **One shared counter here vs. a per-message minimum-interval table in
  the paper.** `MinCycle<100>` is a single global timer shared across
  every mailbox/ID pair. The paper's Table I "Minimum intervals of
  sending messages" register is described as covering "the CAN ID and
  corresponding [] minimum time intervals" (i.e., a table, one interval
  per message type) — the paper itself flags that a single shared
  interval is not necessarily the right model once "there are multiple
  messages from this ECU" (Sec. V-C-2). This example's one-counter
  simplification matches the paper's simplest framing (a single message
  type gated by a single interval) but not its multi-message caveat.
- **Confirms the open `tick`-argument question — genuinely not a fair
  stand-in as built.** The paper is explicit that its minimum-
  transmission-cycle counter is real hardware: "embedded as a bit/time
  unit counter on the CAN network. The counter starts to count up after
  the initial transmission request is generated" (Sec. V-A) — an actual
  free-running counter clocked off CAN bus bit timing, not a value
  supplied by the transmitting software. `MinCycle<>` here takes `tick`
  as a plain function argument the *caller* provides, which is not the
  same guarantee: nothing stops a compromised caller from passing a
  stale or fabricated `tick`. One thing this example does get right
  structurally: like the paper's counter, `MinCycle<>`'s `armed` flag
  means the *first* transmission is never blocked for lack of history —
  the paper states this explicitly too ("the initial transmission
  request is not protected by the minimum transmission cycle counter").
  Wiring a genuine free-running counter into `canDisablerTop` (instead
  of a caller-supplied tick) is now a concrete, paper-grounded follow-up
  rather than an open question with no source to resolve it against.
- **The paper's third, separate axis this example doesn't have at all:
  measured processing-time overhead.** Sec. V-B measures the disabler's
  added latency on real FPGA hardware directly (max 6.25µs after a
  transmission request, "within 3 to 4 bits at 500kbps") — a testbench/
  hardware timing measurement, not a synthesis-time estimate. Nothing in
  this example measures that; Bambu's control-step/cycle counts (see
  [Results](#results-verified-not-estimated) above) are a schedule, not
  a timed measurement, and are called out as such in
  [Not done](#not-done) below independent of this comparison.

## Not done

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
- **Two paper-grounded follow-ups, not attempted here** (see
  [Comparison to the CAN Disabler paper](#comparison-to-the-can-disabler-paper)):
  split `Allow<Mailbox,Id>` into the paper's two genuinely independent
  evaluations (per-mailbox enable, then separate CAN-ID whitelist)
  instead of one combined pair-check; and replace the caller-supplied
  `tick` argument with a real free-running counter driving
  `canDisablerTop`, matching the paper's bit/time-unit hardware counter.
