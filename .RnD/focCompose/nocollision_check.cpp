// Negative test: this file is EXPECTED TO FAIL TO COMPILE. It demonstrates
// that HAPI core's general NoCollision diagnostic (rules.h) now catches, as
// a compile error, the exact real hazard focCompose.cpp's own
// static_assert(decltype(...)) pair could previously only detect after the
// fact (see HANDOFF.md, "ordinary C++ name hiding"): Encoder/BLDCDriver3PWM/
// MotorTerminal folded into one Chain<> hide MotorTerminal's void init()
// behind BLDCDriver3PWM's int init(), silently, with no diagnostic of its
// own. Kept as its own file (not added to focCompose.cpp) because that
// file's own run_tests.sh asserts a CLEAN build -- it demonstrates the
// collision existed, not that it's now caught.
#include "focAPI.h"
#include "stub_foc.h"

using namespace focCompose;
using MotorTerminal = SensorAPI;

HAPI_DETECT_MEMBER(init); // -> HapiMember_init, this scope (matches HAPI/tests/compile_tests.cpp's own placement)

static_assert(hapi::NoCollision<HapiMember_init,
  hapi::Chain<Encoder, BLDCDriver3PWM, MotorTerminal>>,
  "expected to fail: Encoder/BLDCDriver3PWM/MotorTerminal collide on init()");
