// focCompose Step 2: Encoder.h/MagneticSensorSPI.h/BLDCDriver3PWM.h are
// all Arduino.h-coupled at the concrete-class level (confirmed
// empirically -- g++ against a bare Arduino-FOC checkout, no framework:
// both sensor headers fail with "Arduino.h: No such file or directory";
// even the *abstract* Sensor base's non-pure default methods pull in
// Arduino.h transitively through Sensor.cpp -> foc_utils.h, and
// time_utils.cpp calls micros() directly). Stubs both, same posture as
// sensorFusion's StubLight/StubMotion. Names match SimpleFOC's real
// concrete classes for call-site parity; internals are fixed/no-op.
#pragma once
#include "focAPI.h"

namespace focCompose {

  // Stub quadrature encoder -- real part counts pulses via GPIO
  // interrupts (Encoder.cpp). Fixed angle, no real GPIO read.
  struct Encoder {
    template<typename O>
    struct Part : O {
      static float getMechanicalAngle() { return 1.5707963f; } // fixed, pi/2
    };
  };

  // Stub SPI magnetic sensor -- real part reads a 14-bit angle register
  // over SPI (MagneticSensorSPI.cpp). Fixed angle, no real SPI transaction.
  // Deliberately a different bus SHAPE from Encoder's GPIO, same reasoning
  // as sensorFusion's StubLight/StubMotion bus-type spread.
  struct MagneticSensorSPI {
    template<typename O>
    struct Part : O {
      static float getMechanicalAngle() { return 0.7853982f; } // fixed, pi/4
    };
  };

  // Stub 3-PWM BLDC driver -- real part writes timer compare registers
  // (BLDCDriver3PWM.cpp). Fixed/no-op, no real PWM output.
  struct BLDCDriver3PWM {
    template<typename O>
    struct Part : O {
      static int  init() { return 1; }
      static void enable() {}
      static void disable() {}
      static void setPwm(float Ua, float Ub, float Uc) { (void)Ua; (void)Ub; (void)Uc; }
      static void setPhaseState(PhaseState sa, PhaseState sb, PhaseState sc) { (void)sa; (void)sb; (void)sc; }
      static DriverType type() { return DriverType::BLDC; }
    };
  };

}
