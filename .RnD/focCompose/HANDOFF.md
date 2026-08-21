# HANDOFF: FOC device composition via HAPI

Written by a sandboxed agent with AVR cross-compilation only. Steps 1-3
of the `focCompose` plan (robots, Round 1): can `SimpleFOC`
(`simplefoc/Arduino-FOC`)'s `Sensor*`/`BLDCDriver*` runtime pointers
(`linkSensor()`/`linkDriver()`, vtable dispatch every FOC loop iteration)
become compile-time template parameters instead, zero-overhead, for a
combination that's fixed the moment a physical motor+sensor+driver+MCU is
soldered together? Non-goal: not the FOC control loop itself (no Clarke/
Park transforms, no current-loop PID) — scope is the link point only.

Real `Arduino-FOC` cloned fresh (`github.com/simplefoc/Arduino-FOC`,
shallow, HEAD `7f9c45f`) into `../../../../Arduino-FOC` (sibling of `IOP`)
for this round — every interface claim below was read from the actual
checkout, not assumed.

## Step 1 — Contract

`SensorAPI`/`DriverAPI` (`focAPI.h`), no-op-default terminals mirroring
SimpleFOC's real method names exactly (`getMechanicalAngle()`, `init()`,
`update()`, `enable()`, `disable()`, `setPwm()`, `setPhaseState()`,
`type()`) for call-site parity — same role `sensorFusion`'s `SensorAPI`
played, same idiom OneMenu's `ItemAPI` established. `PhaseState`/
`DriverType` enums mirror `FOCDriver.h`'s real ones exactly (values
included), not included directly — see Step 2.

## Step 2 — Reuse real components, stub bus

**Decision point 2 answer: no, confirmed empirically, not just by
grepping `#include` lines.** `sensors/Encoder.h` and
`sensors/MagneticSensorSPI.h` (the plan's two named targets — deliberately
different bus types, GPIO quadrature vs SPI) both fail to compile
standalone:

```
$ g++ -std=c++17 -I Arduino-FOC/src -c enc_test.cpp   # #include "sensors/Encoder.h"
Encoder.h:4:10: fatal error: Arduino.h: No such file or directory

$ g++ -std=c++17 -I Arduino-FOC/src -c spi_test.cpp   # #include "sensors/MagneticSensorSPI.h"
MagneticSensorSPI.h:5:10: fatal error: Arduino.h: No such file or directory
```

**Deeper than the plan anticipated:** the coupling isn't limited to
concrete sensor implementations. `common/base_classes/Sensor.h` itself
(the abstract interface) has zero Arduino dependency (`#include
<inttypes.h>` only) and compiles standalone clean — but `Sensor.cpp`
(needed for any of its *non-pure* default methods: `getMechanicalAngle()`,
`getVelocity()`, `update()`, etc. — only `getSensorAngle()` is pure
virtual) includes `foc_utils.h`, which unconditionally `#include
"Arduino.h"`. `time_utils.cpp` calls `micros()` directly. So even
`Sensor`'s own *default behavior*, not just concrete drivers, is
Arduino-coupled — the abstract interface is portable, its implementation
isn't. Stubbed both (`stub_foc.h`: `Encoder`, `MagneticSensorSPI`, plus
`BLDCDriver3PWM` for the driver side), same posture as `sensorFusion`'s
`StubLight`/`StubMotion` — fixed return values, no real bus transaction.
This is a real, reportable finding about SimpleFOC's portability (stated
in the plan as a real open question, confirmed true, and confirmed to run
deeper than "just the concrete drivers"), not a blocker for this round.

## Step 3 — Compose + disassemble

```cpp
using MotorTerminal = SensorAPI;
using Compose = FocCompose<Encoder, BLDCDriver3PWM>::Part<MotorTerminal>;
```

Decision point 1, Branch A taken (predicted, confirmed sufficient for
this round): a minimal composed motor via plain `Chain<>`, not a fork of
real `BLDCMotor`.

**A second real finding, not predicted by the plan, found by building it
rather than reasoning about it:** SimpleFOC's real `BLDCMotor` holds
`Sensor* sensor;` and `FOCDriver* driver;` as two **separate** pointer
members — `sensor->init()` (void, protected in the real class) and
`driver->init()` (`int`, public, pure virtual in the real class) are two
unambiguously distinct calls, never competing for the same name lookup.
`Chain<Encoder,BLDCDriver3PWM>::Part<MotorTerminal>` is **one flat
inheritance line** (`Encoder::Part<BLDCDriver3PWM::Part<MotorTerminal>>`).
`BLDCDriver3PWM::Part<>::init()` (returns `int`) sits nearer the
composed type's top than `MotorTerminal`'s (`SensorAPI`'s) `void init()`
— ordinary C++ name hiding makes the driver's `init()` the **only** one
reachable as `Compose::init()`. Not a compile error, not an ambiguity —
silent: `SensorAPI::init()` still exists and is still directly callable
by explicit qualification, but is unreachable through `Compose::` once
folded into the same chain as a differently-shaped same-named driver
method. Confirmed by compiling, not just reasoning about it:

```cpp
static_assert(std::is_same<decltype(Compose::init()), int>::value, ...);        // driver's — passes
static_assert(std::is_same<decltype(MotorTerminal::init()), void>::value, ...); // sensor's, still exists — passes
```

**This is the real limit of Branch A**, worth carrying into any Branch B
work: a single linear `Chain<Sensor,Driver>` cannot carry both roles'
same-named methods the way SimpleFOC's two independent pointer members
can. Two ways out, neither attempted this round: (a) hold `Sensor` and
`Driver` as two separately-composed sub-objects/members instead of one
flat chain (closer to `BLDCMotor`'s own two-pointer shape, but per
`StaticList`'s own `sensorFusion` finding, plain data members don't get
EBO the way base-class layering does — would need multiple inheritance
from two distinct empty base classes to keep `sizeof` at 1, which
reintroduces its own naming-ambiguity risk if both bases ever share a
method name, just as a hard error instead of silent hiding); (b) rename
one side's method in the contract, breaking exact call-site parity with
hand-linked SimpleFOC for that one method only. Not decided here — real
design call, flagging it rather than picking one.

Three checks, real avr-g++ 7.3.0 (`atmega328p`), object-file level and a
full real PlatformIO `uno` link:

1. `!__is_polymorphic(Compose)` — holds.
2. `sizeof(Compose) == 1` — **predicted correctly this time**, unlike
   `sensorFusion`'s `StaticList` sizeof finding: `Chain<>::Part` composes
   via base-class inheritance, where EBO is a standard guarantee, not the
   data-member storage `StaticList` uses. Confirmed by static_assert
   (compiles) and by the full build's `RAM: 0 bytes`.
3. Disassembly — **zero calls of any kind**, not just zero indirect:
   `main()` (86 bytes, object-file level; 218B flash / 0B RAM full
   PlatformIO link) fully inlines `Compose::enable()`, `setPwm()`,
   `setPhaseState()`, `type()`, `getMechanicalAngle()`, `init()` — no
   `call`, `icall`, or `callx` at all.

**Decision point 3 answer:** `type()` (mirrors `FOCDriver`'s pure-virtual
getter) costs nothing — confirmed by disassembly, not assumed: the
`volatile DriverType t = Compose::type();` line compiles to a single
fixed immediate load (`ldi r24, 0x01` — `BLDC=1`), no branch, no switch,
no jump table. The temptation the plan flagged (a runtime switch dispatch
sneaking back in) didn't materialize here because the stub's `type()`
body is trivial (`return DriverType::BLDC;`); a real driver's `type()`
would need the same triviality to keep this guarantee — worth restating
for whoever implements a real one.

## Addendum — real STM32 compile, composition mechanism only (LOCAL, not pushed)

Per the CLAUDE.md bridge doc's own flag: no round in this whole line of
work had compiled against real STM32 yet — that gap is now half-closed.
Same `focCompose.cpp`, **completely unmodified** (it has zero chip-I/O
dependency to begin with — no `OneChip` include at all, stubs only),
built for real against `arm-none-eabi-g++` 7.2.1 via a real PlatformIO
build, `platform=ststm32`, `board=bluepill_f103c8` (STM32F103C8T6,
Cortex-M3, 72MHz), `framework=cmsis` — the same board/framework
combination `OneChip/examples/serial`'s own `bluepill_iop` env already
uses, not a new, unverified target choice.

```
RAM:   28 bytes  (0.1% of 20K)
Flash: 268 bytes (0.4% of 64K)
```

Our own object (`src/main.o`, pre-link): **32 bytes, zero calls of any
kind** — not `bl`, not `blx` (ARM's indirect-call-via-register
instruction, the Cortex-M3 analog of AVR's `icall`/Xtensa's `callx8`).
`main()` fully inlines to a handful of register moves and a `bx lr`
return; `Encoder`'s stub `getMechanicalAngle()` return value
(`1.5707963f`) survives only as a raw embedded float literal
(`0x3fc90fda`, confirmed byte-for-byte against Python's own IEEE754
packing of the same value) — the call itself is gone entirely, not just
de-indirected.

Full linked firmware has 2 `blx` instructions — checked, not assumed
harmless: both live inside `__libc_init_array` (newlib's standard
global-constructor-table startup loop, present in essentially every
embedded C++ program, completely unrelated to this composition). Zero
`blx` in our own object.

**This closes the composition-mechanism half of the STM32 gap only** —
same distinction Stage 2's Xtensa `core_check.cpp` drew for ESP32 (“pure
`Chain<>`/`APIOf<>`, no chip I/O” is not the same claim as “real hardware
works”). Still open, unchanged by this: real PWM/ADC timing, real
`OneChip` STM32 driver headers wired into `Encoder`/`BLDCDriver3PWM`
instead of stubs, and Step 4's actual pass/fail (does the composed link
point spin a motor correctly) — all still genuinely local-only, per
Decision point 4. **LOCAL, not pushed** — this addendum, like the rest of
this file, lives only in this sandbox until promoted.

## Addendum — Step 4 prep: real sensor, stub driver (LOCAL, not pushed)

Rui asked to prepare for Step 4 while tracking down the physical board.
Split the prep by risk, deliberately: a real magnetic-angle sensor over
real SPI is safe to build ahead of hardware (read-only, no way to damage
anything); a real 3-phase PWM driver is not — no `OneChip` STM32
PWM/timer driver exists anywhere in this ecosystem yet, and hand-rolling
one from scratch with no hardware or scope to validate against is a real
motor/driver-board damage risk if a timer-mode or dead-time mistake ships
untested. `BLDCDriver3PWM` stays a stub; only the sensor side went real.

### `magnetic_sensor_spi_stm32.h` — real AS5047/AS5048/AS5147 protocol on real hardware

Protocol ported from `Arduino-FOC/src/sensors/MagneticSensorSPI.cpp`'s
real, actual implementation (`AS5147_SPI`/`AS5048_SPI`/`AS5047_SPI`
configs — identical across all three chips): 16-bit SPI command/response,
angle register `0x3FFF`, read bit at 14, even-parity bit at 15, 14-bit
data. Reuses `hw::stm32::f1::Spi<>` — `OneChip`'s existing, real,
register-level SPI1 driver — completely unmodified, the same one already
verified real in the prior STM32 addendum above.

**CS pin (PA4) driven with raw register writes, deliberately not through
existing OneBus/OneChip pin abstractions — checked both, neither covers
this:**

- `oneBus::ChipSelect<>` is AVR-only: its `select()`/`deselect()` use a
  `PORTx`/`DDRx`-adjacent-register trick (`CsPortAddr - 1`) specific to
  AVR's I/O address layout. Doesn't translate to STM32's memory map at
  all.
- `stm32Port.h`'s `STM32Port<>` — **real finding, found by trying to use
  it, not by reading its header comment**: that file's own doc comment
  claims "GPIO register block — identical layout across all STM32
  families," but `STM32Port<>`'s `gpio_regs` is the MODER/OSPEEDR/PUPDR/
  AFR scheme (real for F0/F4/L4/H7) — F1 (the exact chip this whole round
  targets, `bluepill_f103c8`) uses the legacy CRL/CRH scheme, a genuinely
  different register layout, not just a different base address. No `f1::`
  namespace exists in `stm32Port.h` at all — confirmed by grep, not
  assumed from the doc comment being silent. `stm32Spi.h`'s own F1
  `pin_config()` already independently works around exactly this (writes
  `CRL` directly, bypasses `STM32Port<>` entirely) — this file follows
  that same established precedent for PA4's CS function rather than
  routing through a driver that doesn't cover this chip family. Worth a
  real doc fix upstream (the "identical layout" claim), separately from
  adding actual F1 GPIO support to `STM32Port<>` — neither done here,
  both flagged.

**A second real bug, found by trying to build this, not anticipated by
the plan**: `oneChip/clock.h`'s `hw::delay_ms()` platform dispatch only
branches on `__AVR__` and `ARDUINO` — a bare `framework=cmsis` STM32
build (no Arduino, this whole round's target) falls through to the
`else` branch, which assumes a *hosted* environment (`std::chrono`/
`std::this_thread`) and fails to compile (`'std::this_thread' has not
been declared` — no `<thread>` in freestanding ARM). The file's own doc
comment already carves out an AVR "IOP mode" (`#ifndef IOP`, user
supplies `millis()` via their board's SysTick) but never extends that
carve-out to non-AVR bare-metal targets in the `else` branch — it's
missing, not misconfigured. **Not fixed upstream this round** — the
correct fix needs this project's real STM32 SysTick/IOP-mode convention,
which isn't visible from this sandbox; guessing at it risked shipping a
second, differently-wrong assumption on top of the first. Routed around
locally instead: a cycle-counted busy-wait placeholder (`_gap()`, ~72
cycles at 72MHz, ballparked at the AMS datasheet's ~350ns command/
response gap, not calibrated against a real scope) stands in for the
real delay until either `clock.h` gets a real bare-metal-ARM branch or
Rui's own SysTick convention is available to match.

**Also found and fixed, this round's own bug, not a library one**:
`focAPI.h`'s `SensorAPI`/`DriverAPI` never defined `begin()` — every
prior round (`iotComposition`, `sensorFusion`) established `begin()` as
the one method every composed Part answers to uniformly; this round's
Step 1 contract design simply missed it, surfaced immediately by the
compiler once something tried to call `O::begin()` through the chain.
Fixed by adding the same no-op default both `SensorAPI` and `DriverAPI`.

### Real STM32F103 build, real SPI + GPIO register code, sensor side only

`focCompose_stm32.cpp` — same `Chain<Sensor,Driver>::Part<Terminal>`
shape as `focCompose.cpp`, `MagneticSensorAS5x47<Spi>` in place of the
stub `MagneticSensorSPI`, `BLDCDriver3PWM` still stubbed. Real
`arm-none-eabi-g++` 7.2.1, real PlatformIO `bluepill_f103c8`/
`framework=cmsis` build:

```
RAM:   28 bytes   (0.1% of 20K)
Flash: 1796 bytes (2.7% of 64K)
```

`!__is_polymorphic(Compose)` holds. Zero `blx` in our own object
(`src/main.o`) — real symbols only: `Stm32SpiCore::Part<>::spi_init`/
`spi_transfer`, `MagneticSensorAS5x47::Part<>::readRegister`, `main`. No
vtable, no indirect dispatch, real register-level SPI/GPIO code, ready to
flash the moment hardware is available.

**UNTESTED AGAINST REAL HARDWARE, at the time this section was first
written.** Superseded a few hours later the same round — see the next
section: Rui found the board, plugged in a real ST-Link, and this got a
real hardware bring-up session, not just a cross-compile.

## Addendum — real hardware bring-up: ST-Link connected, real bugs found and fixed on real silicon (LOCAL, not pushed)

Real ST-Link/V2 (USB `0483:3748`) detected and used for real, via
PlatformIO's own bundled OpenOCD (`tool-openocd`,
`interface/stlink.cfg` + `target/stm32f1x.cfg`). No motor, encoder, or
driver board attached yet — sensor-side verification only, same scope
split as the prep work above.

**Chip identified for real over SWD**: Cortex-M3 r1p1, target voltage
3.29V, `device id = 0x20036410`, **flash size reported as 128 KiB** — a
real, worth-noting detail: the `bluepill_f103c8` PlatformIO board
definition (and the STM32F103C8T6's own datasheet) advertise 64 KiB;
many real Blue Pill boards ship the same die as the 128 KiB C**B**T6,
just binned/marked as C8T6 — a known community fact, now independently
confirmed against this specific board via `device id`, not assumed. Not
a problem for a ~1.8 KB firmware either way.

**Flashed for real** (`reset halt` → `program ... verify` → `reset run`)
and **read back live peripheral register state via `mdw`** — the thing
no cross-compile or disassembly check can substitute for. First readback
immediately surfaced a real, confirmed bug that static analysis alone
had not caught:

- **SPI1 CR1 register read back as `0x374` → BR=6 (÷128, ~562.5kHz)**,
  not the `SpiMaster<4000000>` config's intended ÷32 (~2.25MHz). Traced
  it, then confirmed by hand-computing `_br(1000000)` == 6, matching
  exactly: `Stm32SpiCore::Part::begin()` (`stm32Spi.h`) unconditionally
  calls `spi_init(1000000u)` as its own hardcoded default, *after*
  `SpiMaster<Speed>::begin()` already called `Base::spi_init(Speed)`
  moments earlier — silently clobbering the `Speed` template parameter
  every time `Spi::begin()` is called through this composition. **This
  is a real, pre-existing bug in `OneChip`'s own `stm32Spi.h`, not
  something introduced this round** — found only because real register
  state was checked, invisible to any static/disassembly check (it's
  purely a runtime initialization-order bug). **Not fixed upstream this
  round** — flagged here, real library code, Rui's call on the right fix
  (an obvious one: `Stm32SpiCore::Part::begin()` shouldn't re-call
  `spi_init()` with its own hardcoded speed when a caller already
  configured one — but not applied blind without review).

- **GPIOA ODR read back as `0x00000010`**, and this file's CS pin code at
  the time wrote to address `0x4001080C` for both `cs_low()`/`cs_high()`
  — that address is **ODR** (offset `0x0C`), not **BSRR** (offset
  `0x10`). Real, confirmed-on-silicon bug: `cs_low()` (`= 1<<4`) actually
  drove PA4 **HIGH**; `cs_high()` (`= 1<<20`, landing in ODR's reserved
  upper half) actually drove it **LOW** — polarity inverted from intent
  — and every call **overwrote the entire ODR register**, clobbering any
  other GPIOA output pin's state, since it was a whole-register write
  (`=`), not BSRR's atomic per-bit set/reset. **This one is this round's
  own bug** (introduced writing `magnetic_sensor_spi_stm32.h`'s first
  version), caught immediately by the same readback that found the
  `stm32Spi.h` issue above.

**Fixed by rewriting the CS pin as a real OneBit/OnePin composition
instead of hand-rolled register pokes**, per Rui's direction mid-session
("OneBit has bit inversion, compose with it for reverse logic") — not
just patching the address constant. `stm32f1_port_core.h` adds a
minimal, local `Stm32F1PortCore<Base,RccBit>` exposing exactly the
`bsrr_set`/`bsrr_clr`/`dir_out`/`clockEnable` surface `oneBit::Mask<>`
already knows how to detect and use (`has_bsrr_set_fn`/`has_bsrr_clr_fn`
— same method names `stm32Port.h`'s `STM32Port<>` already uses, so this
is a drop-in for real F1 support there later, not a divergent one-off).
`CsPin = APIOf<Stm32OutPin, Inverted<>, Mask<Pins<4>>, Stm32F1PortCore<...>>`
— active-low CS composed as `Inverted<>` over an ordinary active-high
`Mask<>` pin, reusing OneBit's own tested `get`/`set`/`on`/`off`
machinery (which already routes `on()`/`off()` through `bsrr_set`/
`bsrr_clr` atomically) instead of re-deriving BSRR semantics by hand a
second time. `CsPin::on()` now means "logically selected" (→ physically
LOW), `CsPin::off()` means "logically idle" (→ physically HIGH) — correct
polarity, and non-destructive to any other GPIOA pin, both **confirmed
on real silicon**, not just by re-reading the new code:

- Rebuilt, reflashed, sampled `GPIOA` (`CRL`/`CRH`/`IDR`/`ODR`) across 6
  live halts mid-loop: `CRL` consistently `0xb4b34444` (PA4's nibble =
  `0x3`, GP push-pull output — correct and stable across every sample;
  PA5/6/7's SPI config nibbles, `b`/`4`/`b`, also stable and untouched);
  `ODR`'s bit4 read `0` in every sample (`CsPin::on()`, physically LOW,
  during the SPI-transfer-dominated part of the loop).
- To positively confirm the HIGH half too (six live samples never caught
  it — the LOW phase dominates the loop's time budget, ~28µs of real SPI
  clocking per read vs. a ~1µs gap), rebuilt once more with the firmware
  parked in an explicit stable idle state (`CsPin::off()` then an
  infinite empty loop) specifically so a halt would deterministically
  land there. Reflashed, read back: `ODR = 0x0000a010` — **bit4 set,
  confirmed HIGH**. Both polarities now independently confirmed against
  real register state, not inferred from one side only.

**What's still genuinely unverified** (no sensor chip attached yet):
whether an actual AS5047/AS5048/AS5147-class chip responds correctly to
this exact command sequence, whether `_gap()`'s placeholder timing is
anywhere close to right, and whether PA4 is actually free/correctly
wired as CS on whatever board this ends up connected to (assumed here,
not confirmed against a real schematic) — the CS *pin control* itself is
now hardware-verified; the *sensor protocol* riding on top of it is not,
yet.

## Addendum — real PWM driver, real hardware, still no motor/board connected (LOCAL, not pushed)

Rui: "build pwm drivers we will need them for sure" — moved `BLDCDriver3PWM`
from stub to real STM32 TIM1 register code, same safe posture as the
sensor side: verify entirely via register readback, connect to an actual
motor/driver board later, deliberately.

**Checked SimpleFOC's real `BLDCDriver3PWM.cpp` before writing anything**
(`Arduino-FOC/src/drivers/BLDCDriver3PWM.cpp`) — real finding that
changed the plan: this driver class is simpler than assumed. It's 3
*independent* PWM channels + 3 separate digital enable pins, one per
phase — not 6 complementary outputs with MCU-side dead-time. That's a
different real SimpleFOC class (`BLDCDriver6PWM`), for boards without an
onboard gate-driver IC generating complementary switching + dead-time in
silicon. `BLDCDriver3PWM`'s shape matches exactly the
DRV8302/DRV8305/DRV8316-class driver boards the original plan named — no
MCU-side dead-time tuning needed, meaningfully lower-risk than what was
originally scoped as "not something to freehand." Real default PWM
frequency (25kHz) pulled from SimpleFOC's own `stm32_mcu.h`
(`SIMPLEFOC_STM32_PWM_FREQUENCY`), not invented.

`stm32f1_tim1_pwm3.h` — TIM1 (advanced-control timer, F103's only timer
with the 6 shape complementary channels, used here in plain independent
mode), register-level, no ST HAL, same convention as `stm32Spi.h`/
`stm32Port.h`. PA8/PA9/PA10 = CH1/2/3, edge-aligned, 25kHz, `Arr = 2879`
(exact: `72MHz/25kHz - 1`). Flagged and got right on the first real
readback: **TIM1's `BDTR.MOE` (Main Output Enable) must be set or no
channel outputs anything at all**, even with `CCER`/`CCMR` fully
configured — an advanced-control-timer-specific interlock absent on
general-purpose timers, easy to silently miss.

`bldc_driver_3pwm_stm32.h` — real `BLDCDriver3PWM_STM32`, voltage→duty
conversion matches SimpleFOC's own exactly (`Ua/voltage_power_supply`,
clamped `[0,1]`). Enable pins (PB0/PB1/PB10 — placeholder choice, no
known driver board yet to match real wiring) composed via the *same*
`oneBit::Mask<>` + `Stm32F1PortCore` pattern the CS pin already used and
hardware-verified — not `Inverted<>` this time, enable pins are
active-high (matches `FOCDriver`'s real `enable_active_high` default).

### A second real bug from this port core, found the same way as the first

`Stm32F1PortCore::dir_out()` originally only handled pins 0-7 (`CRL`) —
correct for the CS pin (PA4) but silently wrong for `EnC` on PB10 (needs
`CRH`), and the file's *own header comment already said as much*
("output pins 0-7... not a general-purpose replacement") — used past its
documented limit anyway while composing the driver, then caught by the
exact same discipline that found the first two bugs: read the real
register. `CRH` read back `0x44444444` — pin 10's field still at its
CRL-... reset default (floating input), never actually reconfigured.
`BSRR`/`ODR` writes for `EnC` were landing on a pin still electrically in
input mode — harmless, but wouldn't have driven a real enable line.
Fixed by extending `dir_out()` to handle the full 0-15 range (`CRL` for
0-7, `CRH` for 8-15, same 4-bit-per-pin field shape, two registers
handled explicitly). Rebuilt, reflashed, reread: `CRH` now `0x44444344`
— pin 10's field `0x3`, confirmed fixed on real silicon, not just by
re-reading the new code.

**One more thing explained, not just observed**: `GPIOB.ODR` consistently
showed an extra bit (bit4, `PB4`) set that neither `EnA`/`EnB`/`EnC`
touch. Traced before assuming it was a bug: PB4 is `NJTRST` on STM32F1 —
CMSIS's own `SystemInit()` (linked into every `framework=cmsis` build,
runs before `main()`) configures JTAG-shared pins to a known pull-up-input
state by default. Benign, unrelated to this driver, confirmed via `CRL`
showing PB4's field as `0x8` (input, pull-up/down) rather than the `0x3`
an actual output would show — not left as an unexplained oddity.

### Full real-hardware verification, no motor or driver board connected

Flashed for real (`hw_probe_pwm.cpp`: `init()` → `enable()` →
`setPwm(6.0f, 3.0f, 0.0f)` at a 12V placeholder supply, parked), read
back every relevant register:

| Register | Expected | Read back |
|---|---|---|
| `TIM1.CR1` | `CEN`=1, `ARPE`=1 | `0x81` ✓ |
| `TIM1.CCMR1` | `0x6868` | `0x6868` ✓ |
| `TIM1.CCMR2` | `0x68` | `0x68` ✓ |
| `TIM1.CCER` | `0x111` (CC1E/CC2E/CC3E) | `0x111` ✓ |
| `TIM1.PSC` | `0` | `0` ✓ |
| `TIM1.ARR` | `2879` (`0xB3F`) | `0xB3F` ✓ |
| `TIM1.BDTR` | `0x8000` (MOE) | `0x8000` ✓ |
| `TIM1.CCR1` | `1439` (dc=0.5) | `0x59F`=1439 ✓ |
| `TIM1.CCR2` | `719` (dc=0.25) | `0x2CF`=719 ✓ |
| `TIM1.CCR3` | `0` (dc=0) | `0` ✓ |
| `TIM1.CNT` | actively incrementing | `0xAF`→`0xF5` across two live halts ✓ |
| `GPIOB.CRL`/`CRH` | pins 0,1,10 = `0x3` (output) | confirmed, post-fix ✓ |
| `GPIOB.ODR` | bits 0,1,10 set (enabled) | `0x413` ✓ (bit4 explained above) |

Every value matches hand-calculated expectations exactly — not "close
enough," exact. This is a real, correctly-configured, currently-running
PWM+enable driver on real silicon. **What's still not verified**: nothing
is connected to a real driver board or motor, so none of this confirms
correct *electrical* behavior under load — only that the MCU-side timer
and GPIO configuration is exactly what the code intends. `voltage_power_supply`
(12V placeholder) is unmeasured against any real supply. Dead-time isn't
relevant to this driver shape (see above), but board-specific enable-pin
polarity/wiring (assumed active-high, PB0/PB1/PB10) is still a guess
until a real driver board is on the bench to match against.

## Addendum — Branch A's open design question, resolved (LOCAL, not pushed)

Decision point 1's real limit (`init()` name-hiding, see Step 3 above)
got a proper comparison instead of staying an open flag. Prototyped and
disassembly-checked three alternatives to the flat
`Chain<Sensor,Driver>::Part<Terminal>` shape, cloud-only, no hardware:

1. **Named data members** (`struct FocMotor { SensorPart sensor;
   DriverPart driver; };`) — `.sensor.init()`/`.driver.init()` are two
   different members, unambiguous by construction, zero C++ lookup
   subtlety involved at all.
2. **Multiple inheritance**, `struct : SensorPart, DriverPart {}`, with
   *distinct* terminals (`SensorAPI`/`DriverAPI` — no shared base, so no
   diamond, unlike an earlier same-terminal test that did hit one) —
   recovers `sizeof==1` (EBO), and turns the `init()` collision from
   *silent* shadowing into a **hard ambiguity error** on unqualified
   access — genuinely safer than the original flat chain, but still
   needs explicit `obj.SensorPart::init()`-style qualification to use.
3. **`Decor<>`/`Hidden<>`-style explicit redirect** (OneMenu's real,
   production-proven pattern, `OneItem/include/oneItem/oneItem.h`):
   stay in one linear chain, hand-write the specific colliding method to
   forward past the shadowing layer to a named continuation. Not
   prototyped fresh here — already real, already working code in the
   ecosystem, discussed and traced to source rather than re-derived.

All three dispatch with zero overhead (0 indirect calls, disassembly
confirmed for 1 and 2). **Named data members won**, and not just as the
safe default: `.sensor.`/`.driver.` is the one variant that actually
matches real SimpleFOC's own `sensor->`/`driver->` two-pointer-member
call shape — Branch B's eventual goal was always drop-in compatibility
with that shape, and named members get there directly instead of via a
workaround. The 1-byte EBO loss (2 bytes vs. Chain's 1) is a rounding
error next to that.

**Applied for real**, not just decided on paper: `focMotor_stm32.cpp`
combines both real, hardware-verified pieces from this round —
`MagneticSensorAS5x47<Spi>` (real SPI + CS) and `BLDCDriver3PWM_STM32`
(real TIM1 PWM + enable pins) — as `FocMotor{ SensorPart sensor;
DriverPart driver; }`, superseding `focCompose.cpp`/`focCompose_stm32.cpp`'s
flawed flat-chain shape. Real PlatformIO `bluepill_f103c8` build:

```
RAM:   28 bytes   (0.1% of 20K)
Flash: 2708 bytes (4.1% of 64K)
```

`!__is_polymorphic(FocMotor)` holds, zero `blx` in our own object, real
symbols only (`MagneticSensorAS5x47::readRegister`, `Stm32SpiCore::
spi_init`/`spi_transfer`, `Stm32F1Tim1Pwm3::begin`, `BLDCDriver3PWM_STM32::
setPwm`, two separate `Stm32F1PortCore::dir_out` instantiations — one per
GPIO base, CS on GPIOA and enables on GPIOB). Flashed for real, read back
every subsystem's registers **together, for the first time** (previously
verified separately): `GPIOA.CRL`/`CRH` confirms CS (PA4=`0x3`), SPI1
(PA5/6/7=`b`/`4`/`b`), and PWM (PA8/9/10=`b`/`b`/`b`) all correctly
co-configured with zero pin/field conflicts; `SPI1.CR1`, `TIM1.CR1`/
`ARR`/`BDTR` all match their previously-verified individual values
exactly, unchanged by being combined into one object. The resolved
design works end to end on real silicon, not just in isolation.

## Files in this round

- `focAPI.h` — Step 1's `SensorAPI`/`DriverAPI` contracts + mirrored enums.
- `stub_foc.h` — Step 2's `Encoder`/`MagneticSensorSPI`/`BLDCDriver3PWM` stubs.
- `focCompose.cpp` — Step 3's composition, the two real findings' pinning
  `static_assert`s, and the `main()` exercised for disassembly.

## TODO for local agent (Steps 4-5, cannot be done in this sandbox)

Real BLDC motor, real encoder or magnetic sensor, real driver board
(TMC6200/DRV8316-class), real STM32 MCU (SimpleFOC's most-supported
platform, and the realistic target — no STM32 or real ESP32-framework
compile has happened in *any* round of this composition work yet, Xtensa
core-only checks aside). Decision point 4 (does the composed link point
actually spin a motor correctly — PWM update rate, ADC sample alignment)
is this round's real pass/fail, not the composition-mechanism proof this
sandbox already established. Three-row marginal-cost table, same
discipline as every prior round.
