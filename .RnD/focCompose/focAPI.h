// focCompose Step 1: SensorAPI/DriverAPI no-op-default terminals, same
// role as sensorFusion's SensorAPI and OneMenu's ItemAPI. Method names
// mirror SimpleFOC's real Sensor/FOCDriver/BLDCDriver interfaces exactly
// (Arduino-FOC/src/common/base_classes/{Sensor,FOCDriver,BLDCDriver}.h)
// for call-site parity -- HAPI needs no particular names, this is purely
// so a FocCompose<>-composed motor's method calls read identically to
// hand-linked SimpleFOC code.
//
// PhaseState/DriverType mirror FOCDriver.h's real enums exactly, values
// included -- not included directly: FOCDriver.h/BLDCDriver.h both
// #include "Arduino.h", confirmed Arduino-coupled at the concrete-class
// level (see focCompose HANDOFF, Step 2).
#pragma once
#include <hapi/hapi.h>
#include <stdint.h>

namespace focCompose {

  enum PhaseState : uint8_t { PHASE_OFF=0, PHASE_ON=1, PHASE_HI=2, PHASE_LO=3 };
  enum DriverType { UnknownDriver=0, BLDC=1, Stepper=2, Hybrid=3 };

  // Real Sensor::init() is protected in SimpleFOC (only Sensor's own
  // subclasses/BLDCMotor call it); kept public here so this minimal
  // composition proof can exercise it directly from one top-level test.
  struct SensorAPI {
    static void  begin() {}
    static void  init() {}
    static void  update() {}
    static float getMechanicalAngle() { return 0.0f; }
  };

  struct DriverAPI {
    static void        begin() {}
    static int        init() { return 1; }
    static void       enable() {}
    static void       disable() {}
    static void       setPwm(float, float, float) {}
    static void       setPhaseState(PhaseState, PhaseState, PhaseState) {}
    static DriverType type() { return DriverType::UnknownDriver; }
  };

  template<typename... OO>
  using FocCompose = hapi::Chain<OO...>;

}
