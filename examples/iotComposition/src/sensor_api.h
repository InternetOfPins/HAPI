// sensorFusion Step 1: SensorAPI<Cfg> -- no-op-default terminal every
// concrete sensor Part answers to, same shape/role as OneItem's
// ItemAPI<Cfg> (OneItem/include/oneItem/oneItem.h).
//
// Decision point 1 (Branch B, taken): no uniform Reading type. Each
// sensor keeps its own native return type (float degC, raw ADC counts,
// bool) -- this header adds nothing to unify that. SensorAPI exists only
// as a shared fallback root so new stub sensors don't each need their
// own private terminal struct (existing components AHT/PCA9685 already
// have one -- SensorDef/PwmDef -- and are left untouched; this is for
// sensors written new in this round only).
#pragma once

namespace oneIO::sensor {

  template<typename Cfg = void>
  struct SensorAPI {
    static void begin() {}
  };

}
