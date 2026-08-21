// focCompose Step 3: compose + disassemble. Decision point 1, Branch A
// (predicted, taken): a minimal composed motor proving the link point,
// not forking real BLDCMotor. Literal plan shape:
//   using Compose = FocCompose<Encoder, BLDCDriver3PWM>::Part<MotorTerminal>;
#include "focAPI.h"
#include "stub_foc.h"
#include <hapi/base.h> // std::is_same -- no <type_traits> on AVR, base.h
                        // already __has_include-gates this the same way
                        // staticList.h's own AVR fix does

using namespace focCompose;

// MotorTerminal = SensorAPI: the composed chain is Encoder::Part<
// BLDCDriver3PWM::Part<MotorTerminal>> -- Encoder overrides only
// getMechanicalAngle(), BLDCDriver3PWM overrides init/enable/disable/
// setPwm/setPhaseState/type, so MotorTerminal only ever needs to supply
// update() as a genuine fallback. Reused SensorAPI directly for that.
using MotorTerminal = SensorAPI;

using Compose = FocCompose<Encoder, BLDCDriver3PWM>::Part<MotorTerminal>;

// ---- Check 1: no vtable ----
static_assert(!__is_polymorphic(Compose), "Compose must not be polymorphic");

// ---- Check 2: sizeof, predicted near-zero (Chain-style layered
// composition via base-class inheritance, not a StaticList typelist --
// EBO should apply the same way it did for stage1_avr's IotDevice) ----
static_assert(sizeof(Compose) == 1, "Compose should be empty (EBO across layers)");

// ---- REAL FINDING, not predicted by the plan: a single linear Chain
// collapses Sensor's and Driver's same-named init() into ONE visible
// symbol. SimpleFOC's real BLDCMotor holds `Sensor* sensor;` and
// `FOCDriver* driver;` as two SEPARATE pointer members -- sensor->init()
// and driver->init() are two unambiguously distinct calls, never
// competing for the same name lookup. Chain<Sensor,Driver>::Part<T> is
// ONE flat inheritance line: BLDCDriver3PWM::Part<T>::init() (returns
// int) sits nearer the top than MotorTerminal's (=SensorAPI's) void
// init(), so ordinary C++ name hiding makes the driver's init() the ONLY
// one reachable through Compose:: -- not a compile error, not ambiguous,
// just silent: SensorAPI's own init() becomes unreachable by name once
// folded into the same chain as the driver's differently-shaped init().
static_assert(std::is_same<decltype(Compose::init()), int>::value,
  "Driver's int init() shadows Terminal's (Sensor-shaped) void init() -- "
  "confirms a single linear Chain cannot carry both role's same-named "
  "methods the way SimpleFOC's two separate pointer members can");

// Sensor's own init() (from MotorTerminal/SensorAPI) is NOT reachable as
// Compose::init() anymore -- it's still callable, but only by bypassing
// name hiding via explicit qualification (proves it's hidden, not lost):
static_assert(std::is_same<decltype(MotorTerminal::init()), void>::value,
  "MotorTerminal::init() (Sensor-shaped) still exists, just hidden from Compose::init()");

// ---- Decision point 3: does type() (mirrors FOCDriver's pure-virtual
// getter) cost anything when Chain-composed instead of vtable-dispatched?
// Not constexpr in the stub (matches the real FOCDriver method not being
// one either), so checked via disassembly instead of static_assert --
// see focCompose HANDOFF for whether it folds to a fixed load or costs
// a real branch/switch.

int main() {
  Compose::enable();
  Compose::setPwm(1.0f, 2.0f, 3.0f);
  Compose::setPhaseState(PHASE_ON, PHASE_ON, PHASE_ON);
  volatile DriverType t = Compose::type();
  volatile float angle = Compose::getMechanicalAngle(); // Encoder's, reachable (no collision on this name)
  volatile int initResult = Compose::init();             // driver's int init(), per the finding above
  (void)t; (void)angle; (void)initResult;
  return 0;
}
