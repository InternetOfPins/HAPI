// sensorFusion Step 3: stub sensor Parts, same posture as Stage 2's
// stub SDK -- fixed return values, no real I2C/SPI/GPIO transaction.
// Proves composition and codegen only, not real bus behavior (that's
// Step 4, LOCAL, real hardware).
//
// Deliberately no ADC/GPIO bus concept invented here (none exists yet
// in OneBus, and inventing one is out of this round's scope) -- the
// bus template param is an unconstrained tag, unused by the stub body.
#pragma once
#include <hapi/hapi.h>
#include "sensor_api.h"

namespace oneIO::sensor {

  // Stub light sensor -- fixed mid-scale ADC count, no real analog read.
  template<typename AdcBus>
  struct StubLight {
    template<typename O>
    struct Part : O {
      static uint16_t read() { return 512; }
    };
    using Api = hapi::APIOf<SensorAPI<>, StubLight<AdcBus>>;
  };

  // Stub PIR motion sensor -- fixed idle reading, no real GPIO read.
  template<typename GpioPin>
  struct StubMotion {
    template<typename O>
    struct Part : O {
      static bool read() { return false; }
    };
    using Api = hapi::APIOf<SensorAPI<>, StubMotion<GpioPin>>;
  };

}
