// sensorFusion Steps 2-3: reuse oneHLS::StaticList directly (Step 2) to
// compose real Aht<>::Api alongside two stub SensorAPI-terminated Parts
// (Step 3) -- a typelist of already-composed, heterogeneous sensor
// devices, sharing no common base beyond the accident of Chain/Part
// composition each already went through independently.
//
// Real AVR target (atmega328p), same shape as stage1_avr.cpp: only
// AHT is real hardware-facing (I2C, unchanged from Stage 1); StubLight/
// StubMotion perform no bus transaction at all (see stub_sensors.h).
#include <hapi/hapi.h>
#include <oneData/oneData.h>
#include <chips/avr/avrDevice.h>
#include <oneIO/sensor/aht.h>
#include <oneHLS/staticList.h>
#include "stub_sensors.h"

using namespace hapi;

using I2c = hw::avr::mega::Twi<>;

// AhtDevice deliberately does NOT use AHT<I2c>::Api: that alias closes
// over AHT's own private terminal (SensorDef), which has an explicitly
// deleted default constructor (SensorDef() = delete; -- a defensive
// "static-methods-only, never instantiate me" marker also present on
// PCA9685's PwmDef). Stage 1 never triggered it: it only ever called
// AHT<I2c> through static methods on the composed TYPE, never created
// an instance. StaticList<> DOES need an instance (Head head{};, since
// visit(i,fn) calls fn(head) on a real object) -- so AHT is reused here
// exactly as Stage 1 used it, as a raw Chain layer under a real
// (default-constructible) terminal, not through its own ::Api.
// Zero changes to aht.h; same header, different composition entry point.
using AhtDevice    = Chain<oneIO::sensor::AHT<I2c>>::Part<oneIO::sensor::SensorAPI<>>;
using LightDevice  = oneIO::sensor::StubLight<void>::Api;
using MotionDevice = oneIO::sensor::StubMotion<void>::Api;

using Fusion = oneHLS::StaticList<AhtDevice, LightDevice, MotionDevice>;

static Fusion fusion;

// Decision point 3, check 1: no vtable across a StaticList of already-
// composed (layered Chain/Part) sensor Parts, not just a StaticList of
// plain leaf types.
// std::is_polymorphic isn't in HAPI's avr_std.h shim (AVR has no
// libstdc++ <type_traits> at all) -- __is_polymorphic is the same GCC/
// Clang builtin std::is_polymorphic itself is defined in terms of,
// used directly so this check runs identically on AVR and native.
static_assert(!__is_polymorphic(Fusion), "Fusion must not be polymorphic");

// Decision point 3, check 2, measured (avr-g++ 7.3.0/atmega328p, real
// build, via a deliberately-undefined-template trick that forces the
// compiler to print the real value in a diagnostic): sizeof(Fusion) ==
// 3, one byte per element, for 3 fully stateless sensor devices. REAL
// FINDING: EBO does NOT survive when StaticList's own Head/Tail are
// DATA MEMBERS, not base classes. Chain<>::Part achieves zero-overhead
// composition via base-class inheritance, where EBO is a standard
// *guarantee*. StaticList<O,O2,OO...> stores `Head head; Tail tail;` as
// ordinary members -- C++ guarantees every member (unlike an empty
// base) a distinct address, so N stateless elements cost N bytes, not
// the near-zero Chain<> achieves layer-over-layer. [[no_unique_address]]
// would fix this but needs C++20, above this ecosystem's -std=c++17
// ceiling (project_cpp_standard_ceiling). Small in absolute terms here
// (3 bytes), but the "no new core-contamination" track record from
// prior rounds does NOT extend to "StaticList's zero-overhead guarantee
// matches Chain<>'s" -- it doesn't, and this is why.
static_assert(sizeof(Fusion) == 3, "StaticList-of-3-stateless-devices sizeof regressed from measured 3");

struct DeviceTerminal { static void begin() {} };

int main() {
  fusion.head.begin();          // AhtDevice (compile-time getAt<0> shape)
  fusion.tail.head.begin();     // LightDevice
  fusion.tail.tail.head.begin();// MotionDevice

  // visit() with a compile-time-constant i -- the thing Step 3 checks
  // disassembles to a direct call, not a jump table. The generic fn is
  // the one thing every sensor Part here actually shares uniformly
  // (begin(), the existing Chain/Part convention) -- per Decision point 1
  // (Branch B), read()/measure() stay native-typed and are NOT called
  // through visit(), only through each device's own accessor, directly.
  auto beginner = [](auto& s) { s.begin(); };
  fusion.visit(0, beginner);
  fusion.visit(1, beginner);
  fusion.visit(2, beginner);

  // Native-typed accessors, called directly, outside visit() -- proves
  // Branch B's "zero uniformity imposed" claim actually holds end to end,
  // not just at the visit() call site.
  bool measured = AhtDevice::measure();
  int16_t   tC10 = measured ? AhtDevice::tempC10() : int16_t(0); // native: int16_t, degC x10; 0 if measurement failed
  uint16_t  lux  = LightDevice::read();      // native: uint16_t, raw ADC
  bool      pir  = MotionDevice::read();     // native: bool

  volatile int16_t  sink0 = tC10;
  volatile uint16_t sink1 = lux;
  volatile bool      sink2 = pir;
  (void)sink0; (void)sink1; (void)sink2;

  return 0;
}
