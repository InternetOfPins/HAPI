# HANDOFF: IoT device composition via HAPI

> **Independently re-verified 2026-08-20** (local agent, real toolchains).
> Both flagged bugs confirmed real and fixed in their own repos:
> `OneHLS/include/oneHLS/oneHLS.h`'s AVR `<cstdint>` gate (commit
> `74ce997`) and `OneIO`'s `pca9685.h` stale `@brief` (commit `9cb61c0`),
> both pushed. `stage1_avr.cpp` rebuilt clean via a real PlatformIO `uno`
> env (symlink:// to all 12 sibling libs): 0 `icall`/`callx` confirmed via
> `avr-objdump`, 1252B flash / 17B RAM measured (vs. this handoff's
> claimed 1450B/20B via bare `avr-g++ -Os -std=c++17` with no PlatformIO
> or Arduino framework linked — same build-methodology gap seen again in
> `.RnD/mlComposition`'s Stage 1, not a discrepancy worth chasing:
> PlatformIO's `--gc-sections` discards the unused framework code either
> way, and the composed type only ever calls its own register-level
> `hw::avr::mega::` drivers, never Arduino's `Wire`/`HardwareSerial`).
> `static_assert(sizeof(IotDevice) <= 4)` held. This directory itself
> (`HANDOFF.md` + `stage1_avr.cpp`) was verified the same session it
> arrived but only committed later, once the pattern of also landing the
> `.RnD/` writeup (not just the library fixes it surfaced) was applied
> retroactively, matching `.RnD/blockchainKernelHLS` and
> `.RnD/mlComposition`.

Written by a sandboxed agent with AVR cross-compilation only (no ESP32
toolchain, no STM32 hardware, no MQTT broker, no CAN transceiver), for a
local agent that has all four. Everything marked **verified** below was
actually cloned fresh from `github.com/InternetOfPins/*` today and
actually compiled/linked — not estimated. Everything marked **TODO**
needs your toolchains/hardware and hasn't been run anywhere.

## Origin

Rui's own framing of the experiment: *can a realistic IoT device be
assembled from independently developed components through HAPI, with the
final firmware containing only the behavior actually composed* — and the
real measurement is the pipeline `IoT application → composed C++ object →
generated firmware → ELF/assembly → measured footprint`, not just "HAPI
compiles." Explicit instruction: no new `HAPI-IoT` library — this is a
domain demonstration, same shape as `blockchainKernelHLS` before it
(`.RnD/` scratch → promote to `examples/` once solid, same path
`examples/hls_blockchain_kernel` took).

Per Rui's standing preference, this session's job before writing any new
component was to check what IOP already has and use it. That check
changed the plan more than expected — see below.

## The proposal's 5 boxes vs. what's actually in the ecosystem

The original sketch (Sensors / Network / Storage / Control / Actuators)
reads like 5 blank boxes to fill. They aren't. Four of the five already
have real, working components across `OneChip` → `OneBus` → `OneIO` →
`OneHLS`. Only **Network** is a genuine gap.

| Box | Status | Evidence |
|---|---|---|
| **Sensors** | mostly real | `oneIO::sensor::AHT` (I2C temp+humidity, full protocol impl), `DS18B20` (1-Wire temp, full impl), `MAX31855` (SPI thermocouple, full impl). `MPU6050` (I2C accel/gyro) is a **stub** — method declarations only, no bodies. No pressure sensor. |
| **Storage** | real, and already cross-swappable | `oneIO::eeprom::AT24C` (I2C EEPROM) and `oneIO::storage::SDCard` are explicitly written to share one byte-offset interface so an `EepromStore<>` can use either as backing store — this is *already* the abstraction the proposal asked for, not something to build. `oneChip::flashMem.h`/`avrEeprom.h` sit one layer below as raw chip drivers. |
| **Control** | real, and stronger than assumed | `oneHLS::Pid<Sample,Accum,Kp,Ki,Kd>` is a real library type in `OneHLS/include/oneHLS/oneHLS.h` (not example-only), `Chain`/`APIOf`/`Data<T>`-composed, `.step(error)` surface, already HLS-verified (Bambu + Vitis, post-route Vivado numbers on file for `pid_top`). Usable as plain C++ on an MCU with zero changes — same header, just pick a plain `int16_t`/`int32_t` `Sample`/`Accum` instead of `ac_fixed`/`ap_fixed`. |
| **Actuators** | real | `oneIO::pwm::PCA9685` (I2C 16-ch 12-bit PWM, full impl) works identically on AVR/ESP32/STM32 since it only needs an I2C bus — good uniform actuator across all three demo targets, avoids needing native per-chip PWM/LEDC drivers at all. |
| **Network** | **gap** | No WiFi, MQTT, CoAP, HTTP, or CAN anywhere in `OneChip` or `OneIO` (checked both repos in full). This is the one real box to build. |

This also matters beyond the demo: `HAPI/docs/INDUSTRY.md` already lists
*"CAN, Modbus, NMEA, MQTT, telemetry systems, industrial field buses...
embedded networking stacks"* as an applicable domain, with nothing built
or verified anywhere in the ecosystem to back that up yet. Building the
Network components doesn't just serve this demo — it turns an asserted,
undemonstrated doc claim into a real one.

## What's already verified (this session)

### Stage 1 — AVR composition, real cross-compile, real numbers

`stage1_avr.cpp` (included alongside this file) composes **Sensor + Actuator
+ Network in one `Chain<>`**, **Control as a plain member**, **Storage
called directly** — using only existing headers, zero new component code:

```cpp
using I2c    = hw::avr::mega::Twi<>;
using Serial = hw::avr::mega::Serial0<9600>;
using Sensor = oneIO::sensor::AHT<I2c>;
using Pwm    = oneIO::pwm::PCA9685<I2c>;
using Store  = oneIO::eeprom::AT24C<I2c>;   // not chain-spliced, see below

using IotDevice = Chain<Sensor, Pwm, Serial>::Part<DeviceTerminal>;
static oneHLS::Pid<int16_t, int32_t, 256, 32, 16> heaterPid;
```

Compiled and linked for real `atmega328p` (avr-g++ 7.3.0, `-Os -std=c++17`):

```
AVR Memory Usage
Program:  1450 bytes (4.4% of 32K)
Data:       20 bytes (1.0% of 2K)
```

Zero `callx`/`icall` instructions in the disassembly — the zero-overhead
composition thesis holds with real, independently-authored, cross-repo
components (`OneIO`'s AHT and PCA9685, `OneBus`/`OneChip`'s Uart, `OneHLS`'s
Pid) glued into one type, not just HAPI's own synthetic benchmarks.

**One design note worth carrying into Stage 2/3:** `AT24C` (and `AHT`,
`PCA9685`, most sensor/storage drivers) are plain closed static classes
parameterized by a bus type — they don't have a `template<typename O>
struct Part`, so they're *not* chain-splice-able the way `oneHLS`'s
`Tap`/`Data` are. That's fine and intentional — call them directly
(`Store::write(addr, buf, len)`) rather than forcing everything into one
inheritance chain. Not every component in this ecosystem is the same
composition "grain," and the demo shouldn't paper over that.

### Two small real bugs/gaps found and fixed/flagged along the way

1. **`oneHLS.h` isn't AVR-safe as shipped** — it unconditionally
   `#include <cstdint>`, which doesn't exist in avr-g++'s toolchain (no
   full libstdc++ on AVR, only `<stdint.h>`). One-line fix, same pattern
   `hapi/platform/avr/avr_std.h` already exists to handle:
   ```cpp
   #ifdef __AVR__
     #include <stdint.h>
   #else
     #include <cstdint>
   #endif
   ```
   Verified this fix alone is sufficient — nothing else in `oneHLS.h`
   needed changing to compile clean on AVR. Worth a real PR to `OneHLS`;
   trivial, no behavior change on any other target.
2. **`pca9685.h`'s doc comment is stale** — the `@brief` says
   `setChannel(n, on, off)`, the actual method is `set(ch, duty)` (plus
   `set_raw(ch,on,off)`, `set8`, `full_on`/`full_off`). Not a bug, just a
   one-line doc fix.

### What this does *not* verify

Physical I2C/1-Wire/SPI bus behavior (no hardware attached — this only
proves the types compose and the generated code is register-level, not
that an AHT/PCA9685 chip on the bench responds correctly), ESP32 or STM32
compilation, MQTT, CAN, or any Storage/Actuator config beyond the one AVR
combination above.

## The real gap, concretely: what to build

Same idiom as everything above — a `Logic` struct with `template<typename
I> struct Part : I`, `Data<T>` for state where needed, composed via
`Chain<>`. `oneHLS.h` is the cleanest template to imitate (it's the newest,
most consistently-styled file in the ecosystem).

- **`oneIO::net::Mqtt<...>`** — thin `Part` wrapping a vendor MQTT client
  (PubSubClient over `WiFiClient` is the standard Arduino-ESP32 pairing).
  `publish(topic,payload)` / `subscribe()` surface. ESP32-only for v1.
- **CAN — two layers, both missing:**
  1. `OneChip` needs a raw driver first (`chips/stm32/stm32Can.h`, bxCAN
     registers) — matching the register-map-only style every other
     `OneChip` driver already uses (`avrTwi.h`, `stm32Twi.h`, etc.), not a
     dependency on ST's HAL or libopencm3. **Flagging this as a decision
     point, not assuming it** — see below.
  2. `oneIO::net::Can<...>` `Part` on top, same shape as the MQTT one.
- **Serial (AVR leg):** nothing to build — `avrUart.h`'s `Serial0<>` is
  already exactly this, used as-is in Stage 1 above.

Recommend keeping Network to **one transport per target** for v1 (Serial
on AVR, MQTT on ESP32, CAN on STM32) rather than a swappable multi-transport
stack — the point of this demo is heterogeneous device *configuration*,
not full protocol coverage. CoAP/HTTP from the original sketch: defer,
same reasoning Rui used to cut Bitcoin/Ethereum/wallets from the
blockchain experiment — they'd bury the thesis under domain complexity
without adding to it.

## Staged plan

Stage 0 (skip, already decided by the framing above) and Stage 1 (done,
above) aside:

**Stage 2 — ESP32 + MQTT.** Same application shape as Stage 1: swap `I2c`
to `hw::esp32::...::Twi` (needs checking — `OneChip`'s `esp32Twi.h` exists
but I haven't opened it), Sensor/Actuator/Storage components unchanged
(same headers, same bus-parameterized design — that's the whole point).
Network becomes the new `oneIO::net::Mqtt<>`. This is the stage that
needs the new component from above. TODO: real ESP32 toolchain (PlatformIO
`espressif32` platform), a broker to publish against (local Mosquitto is
enough — doesn't need to be internet-reachable for this to prove the
point).

**Stage 3 — STM32 + CAN.** Same shape again. Network becomes
`oneIO::net::Can<>`, which needs the new `OneChip` CAN driver first. Do
this stage last — it's the only one needing two new pieces (driver + Part)
instead of one, so let the pattern prove itself twice on cheaper ground
first (Stages 1–2). TODO: STM32F103 or F407 (both already have `OneChip`
device files), CAN transceiver + a second node or bxCAN loopback mode for
a no-second-node self-test.

**Stage 4 — the actual measurement.** Automate ELF→section-size extraction
across all three configs (`avr-size`/`arm-none-eabi-size`/ESP32 equivalent),
plus one config with Network `Chain`-spliced out entirely, to make "optional
components disappear when unused" a number, not an assertion. A simple
table (bytes flash / bytes RAM / indirect-call count per config) is the
whole deliverable here — this is the "really interesting measurement"
from Rui's own framing, and everything before it exists to produce
clean inputs for it.

## Decision points needing Rui's call (not assumed here)

- **STM32 CAN driver: hand-rolled bxCAN registers vs. wrapping
  libopencm3.** Given the standing go-to-market framing (target the STM32
  FOSS community, position against libopencm3/stm32-rs on exactly this
  kind of chip-variant problem), hand-rolled register-level seems more
  consistent with what every other `OneChip` driver already does and with
  that framing — but that's a real strategic call, not a technical default,
  and it's Rui's to make.
- **Pressure sensor.** The original sketch says "temp, pressure, accel" —
  right now it's temp (AHT/DS18B20/MAX31855, all real) and accel (MPU6050,
  currently a stub). Worth implementing MPU6050 for real, adding a BMP280
  (I2C, same bus pattern as AHT), both, or neither?
- **Single-transport-per-target vs. swappable stack**, as above —
  recommended default stated, not decided.

## Where this lives

Working location: `.RnD/iotComposition/` inside the `HAPI` checkout,
matching the repo's own convention (same as `.RnD/blockchainKernelHLS/`
before it). Once a stage is solid — compiles clean, native/loopback test
passes, sizes measured — the device demo itself promotes to
`examples/` (matching `examples/hls_blockchain_kernel`'s own graduation),
**not** a new top-level IOP repo, per the original instruction. If the new
`Mqtt<>`/`Can<>` Parts and the CAN chip driver prove broadly reusable
beyond this one demo, *those specific pieces* — not the demo device — are
what graduate into `OneIO`/`OneChip` proper, the same way `Reg<T>`
graduated from `.RnD/` into `HAPI/include/hapi/reg.h` once it was proven.

## Round 3 — Stage 2 (ESP32 + MQTT), with real caveats

Went further than "staged plan" this round: got a real Xtensa/ESP32
cross-compiler (crosstool-NG `esp-14.2.0_20241119`,
`xtensa-esp32-elf-g++ 14.2.0`, pulled from
`github.com/espressif/crosstool-NG` releases — no ESP-IDF or
Arduino-ESP32 core, just the bare compiler) and used it for real. Two
things came out of that, one clean, one that needs a caveat read in full
before quoting the numbers anywhere.

### Core mechanism, verified on a third architecture

Before touching any chip-specific code: does `Chain<>`/`APIOf<>` — no I/O,
pure composition — still fold to nothing on Xtensa, the way it does on
AVR? Yes: a 3-layer `Chain<Weighted<3>,Weighted<7>,Weighted<11>>` compiled
to **42 bytes**, fully inlined, the compiler even strength-reduced the
chained constant multiplies (3×7×11=231) into shift-adds. Zero-overhead
composition now confirmed on three architecturally distinct targets (8-bit
AVR, 32-bit Xtensa; ARM/STM32 still unverified, see Stage 3).

### `oneIO::net::Mqtt<Ssid,Pass,Broker,Port>` — new component, candidate only

Same shape as `oneIO::storage::SDCard` — wraps a vendor Arduino library
(`WiFi.h` + `PubSubClient`), not a from-scratch stack:

```cpp
template<const char* Ssid, const char* Pass, const char* Broker, uint16_t Port = 1883>
struct Mqtt {
  template<typename O> struct Part : O {
    static void begin() { WiFi.begin(Ssid,Pass); /* wait for connect */ client.setServer(Broker,Port); O::begin(); }
    static bool publish(const char* topic, const char* payload) { ... return client.publish(topic,payload); }
  };
};
```

Saved as `mqtt.h.candidate`, not `mqtt.h` — it's new, untested against a
real broker or real Arduino-ESP32 core, and not proposed for promotion
into `OneIO` as-is.

### Stage 2 IotDevice — compiles real, but read this before citing a number

Composed the same shape as Stage 1: `Chain<Sensor, Pwm, Net>::Part<T>`
with `Sensor`/`Pwm`/`Store` **completely unchanged** from Stage 1 (only the
bus type swaps, `hw::esp32::Esp32TwiMaster<>` for `hw::avr::mega::Twi<>`),
`Net` = the new `Mqtt<>`. This compiled clean against the real
`xtensa-esp32-elf-g++`, using a hand-written **API-shape stub** for
`Wire`/`WiFi`/`PubSubClient`/`SPI`/`Arduino.h` (`stub_sdk/` — every
function is a no-op or trivial return, not real driver logic). No real
Arduino-ESP32 core or ESP-IDF here — pulling either in fully wasn't
practical in this sandbox (large repos, and the parts that matter —
`driver/i2c.h`, the WiFi stack — aren't reachable through this session's
network allowlist without the full SDK tree around them).

**What that means for the number:** the compiled object is 186 bytes text
/ 12 bytes bss. That is *not* comparable to Stage 1's real ~1.2–1.5KB — it
measures the composition glue plus our own logic, with the entire driver
layer (I2C bit-banging, WiFi negotiation, MQTT framing) stubbed to
nothing. A real number needs the real SDK. Don't quote 186B as "ESP32
firmware size" anywhere; it isn't one.

**What it does establish:** the same `Sensor`/`Pwm`/`Store` component code
that ran on AVR compiles unchanged against ESP32's bus type — real
cross-architecture portability at the component level, not just in
principle. And the new `Mqtt<>` type-checks and composes correctly
alongside them.

**A real methodology catch, not a result to skip past:** the compiled
object has 6 `callx8` (indirect-call) instructions — on AVR that would
mean real dispatch overhead, so it needed checking, not assuming either
way. Xtensa's `-mlongcalls` ABI (required on ESP32, since flash-mapped
code routinely exceeds the direct-call range) implements *every* call to
a target that might be far away as `l32r` (load address) + `callx8`,
**regardless of whether the target is statically known** — so raw
`callx` counts don't transfer across architectures as a zero-overhead
proxy the way they did AVR-to-AVR. Checked the relocations instead of
trusting the count: all 6 `l32r`s carry an `R_XTENSA_ASM_EXPAND`
relocation to the fixed symbol `delay` (the stub `WiFi.begin()`'s
busy-wait). Static target, ABI-mandated indirection, not real dynamic
dispatch. Zero genuine composition-driven dispatch, same as AVR — but
confirmed by reading the relocations, not by counting an instruction whose
meaning changes with `-mlongcalls`.

### TODO for local agent

Real Arduino-ESP32 core or ESP-IDF, in an environment that already has
either set up (this is likely already true locally — closing this out for
real is probably cheaper for you than it was for this sandbox). Real
broker for an actual `publish()` round-trip. Then the real number, and a
real comparison against Stage 1's.

> **Round 3 independently re-verified 2026-08-20**, TODO closed for real:
> built against the actual local toolchains this handoff assumed but
> didn't have — real `xtensa-esp32-elf-g++` 8.4.0 (crosstool-NG
> `esp-2021r2-patch5`, close enough to the handoff's own 14.2.0 build, not
> identical), real `framework-arduinoespressif32` via PlatformIO
> (`espressif32` platform, `esp32dev` board), real `knolleary/PubSubClient`
> 2.8.0 fetched from the PlatformIO registry — not the stub SDK.
>
> **Found a real bug the stub build couldn't catch**: `stage2_esp32.cpp`
> defined `extern "C" void app_main()` — ESP-IDF's entry point.
> Arduino-ESP32's own `framework/main.cpp` *also* defines `app_main()`
> (it's what calls `setup()`/`loop()`) — linking against the real
> framework throws `multiple definition of 'app_main'`. Never surfaced
> against the hand-written stubs since nothing there competed for the
> symbol. Fixed by renaming to `void setup(){...}` / `void loop(){}`,
> Arduino's actual entry-point convention; also dropped the manual
> `#define ARDUINO 100000` (redefinition warning — the real framework
> already defines it). No other change needed — same composition, same
> `Sensor`/`Pwm`/`Store`/`Mqtt<>` types.
>
> **Real numbers, with the framework's own fixed floor subtracted out**
> (a blank `void setup(){} void loop(){}` sketch on the same board/env):
>
> | | Flash | RAM |
> |---|---|---|
> | blank Arduino-ESP32 sketch | 233185 B | 21032 B |
> | + Sensor/Pwm/Store (no Net) | 266201 B | 21488 B |
> | + Mqtt/WiFi/PubSubClient | 744345 B | 44808 B |
>
> Marginal cost of IOP's own composition (Sensor+Pwm+Store+Pid, real I2C
> driver code, not glue): **+33016 B flash / +456 B RAM**. Marginal cost
> of adding WiFi+MQTT: **+478144 B flash / +23320 B RAM** — that's
> `WiFi.h`+`PubSubClient`'s own footprint (TLS/WiFi negotiation stack),
> not a composition-mechanism cost, and would be paid by any Arduino-ESP32
> sketch using them, HAPI or not. Neither the handoff's stub numbers
> (186B) nor a raw "744KB total" figure is the right one to quote — this
> three-row breakdown is.
>
> **Re-did the indirect-call check against real code**, same discipline
> the handoff's own caveat called for: `callx8` count on the *whole linked
> firmware* is dominated by framework code (682/4768 for the two configs)
> and isn't meaningful. Restricted to just `src/main.cpp.o` (our own
> composed object, pre-link): 58 (no-Net) / 66 (with Net) `callx8`
> instructions, and *every one* resolves via its `l32r` relocation to a
> statically-known symbol — `TwoWire::*`, `delay`, `WiFiSTAClass::begin`,
> `PubSubClient::*`, or our own composed methods (`AHT<>::Part<>::begin`,
> etc.). Zero genuine runtime/data-driven dispatch, matching Round 3's
> stub-build conclusion, now confirmed against the real vendor libraries.
>
> Not independently re-checked: the `Chain<Weighted<3>,Weighted<7>,
> Weighted<11>>` → 42-byte claim above — no source file for it was
> included in the handoff, only the inline description.

## Files in this handoff

- `HANDOFF.md` — this file.
- `stage1_avr.cpp` — the verified Stage 1 composition (compiles clean,
  numbers above). Not yet wrapped in a PlatformIO example structure
  (native + avr environments, README) — that packaging is TODO, matching
  `hls_can_disabler`/`hls_core_components`'s existing example shape.
- `stage2_esp32.cpp` — Round 3's Stage 2 composition, `app_main()` ->
  `setup()`/`loop()` fixed post-verification (see addendum above), real
  numbers in the addendum table.
- `mqtt.h.candidate` — the new `oneIO::net::Mqtt<>` component. Candidate
  only, not promoted into `OneIO`.
