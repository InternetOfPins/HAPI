# iotComposition

Can a realistic IoT device be assembled from independently developed
components through HAPI, with the final firmware containing only the
behavior actually composed? Three device configurations, three real cross
targets, glued from `OneChip`/`OneBus`/`OneIO`/`OneHLS`/`OneData` components
that were never written with each other in mind — the real measurement is
the pipeline `application → composed C++ object → generated firmware →
ELF/assembly → measured footprint`, not just "HAPI compiles."

This directory is the promoted, buildable result of an earlier local
exploration — matching `examples/hls_blockchain_kernel`'s own graduation
path.

## What it is

Three environments, three separate `src/*.cpp` (selected per env via
`build_src_filter` — this is one project, not three copy-pasted examples):

### `stage1_avr` — Sensor+Actuator+Network on real AVR

```cpp
using IotDevice = Chain<Sensor, Pwm, Serial>::Part<DeviceTerminal>;
```

`Sensor` = `oneIO::sensor::AHT<I2c>` (real I2C temp/humidity), `Pwm` =
`oneIO::pwm::PCA9685<I2c>` (real I2C PWM), `Serial` = `hw::avr::mega::Serial0<>`
stands in for "Network" on this leg. `Control` (`oneHLS::Pid<>`) held as a
plain member — same header used for HLS synthesis elsewhere in this
ecosystem, just parameterized with plain `int16_t`/`int32_t` here instead of
`ac_fixed`/`ap_fixed`. `Storage` (`oneIO::eeprom::AT24C<I2c>`) called
directly, not chain-spliced — see the design note in `stage1_avr.cpp` for
why (its terminal has a deliberately deleted default constructor; usable
as a static-methods-only chain layer, not as a standalone instance).

```
RAM:   17 bytes  (0.8% of 2K)
Flash: 1252 bytes (3.9% of 32K)
```

0 `icall`/`callx` in the disassembly — real, independently-authored,
cross-repo components glued into one type, zero indirect dispatch.

### `stage2_esp32` — same composition, Network becomes MQTT

`Sensor`/`Pwm`/`Store` completely unchanged from `stage1_avr` (only the I2C
bus type swaps, `hw::esp32::Esp32TwiMaster<>`), `Net` becomes
`oneIO::net::Mqtt<>` (`include/oneIO/net/mqtt.h` — a thin `WiFi.h`+
`PubSubClient` wrapper, same shape as `oneIO::storage::SDCard`). Builds
against the real `framework-arduinoespressif32` core and real
`knolleary/PubSubClient`, not a stub SDK.

```
                               | Flash    | RAM
blank Arduino-ESP32 sketch     | 233185 B | 21032 B
+ Sensor/Pwm/Store (no Net)    | 266201 B | 21488 B
+ Mqtt/WiFi/PubSubClient       | 744345 B | 44808 B
```

Marginal cost of IOP's own composition (real I2C driver code, not glue):
**+33016 B flash / +456 B RAM**. Marginal cost of adding WiFi+MQTT:
**+478144 B flash / +23320 B RAM** — that's `WiFi.h`/`PubSubClient`'s own
footprint, paid by any Arduino-ESP32 sketch using them, HAPI or not, not a
composition-mechanism cost.

`callx8` count on the composed object alone (not the whole linked firmware,
which is dominated by framework code): 58 (no Net) / 66 (with Net), every
one resolving via its `l32r` relocation to a statically-known symbol
(`-mlongcalls` ABI turns every far call — even to a known target — into
`l32r`+`callx8` on Xtensa; checked the relocations, not just the count).
Zero genuine runtime/data-driven dispatch.

`oneIO::net::Mqtt<>` is example-local (`include/oneIO/net/mqtt.h`), not
promoted into `OneIO` proper — compiles and links against the real vendor
libraries, but no live broker round-trip has been verified yet.

### `sensorFusion` — a typelist of heterogeneous sensor devices

Tests the other half of the architecture the OneMenu/OneHLS StaticBody/
StaticList work found: a typelist of independently-composed sibling
components sharing one contract, applied to sensor hardware instead of menu
Items or FIR taps. Not a sensor-fusion algorithm (no Kalman filter) —
"fusion" means reading N heterogeneous sensors through one shared contract,
dispatched at compile time, zero runtime indirection.

```cpp
using Fusion = oneHLS::StaticList<AhtDevice, LightDevice, MotionDevice>;
```

`AhtDevice` reuses the real `AHT<I2c>` (same I2C protocol code as
`stage1_avr`), `LightDevice`/`MotionDevice` are stub `SensorAPI`-rooted
Parts (`src/stub_sensors.h`, fixed return values, no real bus transaction).
Each sensor keeps its own native return type (`int16_t` degC×10, `uint16_t`
raw ADC, `bool`) — the only thing called uniformly through `visit(i,fn)` is
`begin()`; native reads go through each device's own accessor directly.

```
RAM:   3 bytes  (0.1% of 2K)
Flash: 668 bytes (2.1% of 32K)
```

0 `icall`/`callx`, every `call` resolving via `R_AVR_CALL` relocation to a
fixed symbol (checked, not just counted).

**Real finding, not just corroboration:** `sizeof(Fusion) == 3` — one byte
per element, for 3 fully stateless sensors. `StaticList<O,O2,OO...>` stores
`Head head; Tail tail;` as ordinary *data members*, not base classes — EBO
is a standard guarantee for empty *base* subobjects only, and C++ guarantees
every data member a distinct address regardless of emptiness.
`Chain<>::Part`'s own zero-overhead composition comes from base-class
inheritance specifically, where EBO applies layer over layer. `StaticList`'s
guarantee is genuinely weaker: **N bytes minimum for N heterogeneous
elements**, not Chain's near-zero. Small here (3 bytes), but a real caveat
on `StaticList`'s zero-overhead claim, not folded silently into "it worked."

## Build it

```sh
pio run -e stage1_avr      # atmega328p, real numbers above
pio run -e stage2_esp32    # esp32dev, real Arduino-ESP32 core + PubSubClient
pio run -e sensorFusion    # atmega328p, StaticList composition
```

## What's not verified yet (real hardware needed)

Physical I2C/bus behavior (no hardware attached — this proves the types
compose and the generated code is register-level, not that a chip on the
bench responds correctly), a real second sensor on a bus type other than
I2C (SPI or GPIO/analog) through `sensorFusion`'s `StaticList`, a live MQTT
broker round-trip, STM32+CAN (never attempted — the original plan's Stage
3, network transport for that leg still needs an `OneChip` CAN driver
first).
