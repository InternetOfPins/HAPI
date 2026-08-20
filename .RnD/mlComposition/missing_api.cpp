// Optionality gap 2, case "no API -> compile failure": reusing Experiment
// 1's real Calibrate/Sensor pair rather than inventing a contrived example
// for the ML net specifically -- Calibrate calls O::tempC10(), so composing
// it without a tempC10()-providing Sensor should fail to compile, cleanly.
#include <stdint.h>
#include <hapi/hapi.h>
using namespace hapi;

template<int16_t OffsetX10>
struct Calibrate {
  template<typename O>
  struct Part : O {
    static int16_t calibratedTempC10() { return int16_t(O::tempC10() + OffsetX10); }
  };
};

struct EmptyTerminal { static void begin() {} }; // deliberately no tempC10()

using BrokenPipeline = Chain<Calibrate<-3>>::Part<EmptyTerminal>;

int main() {
  return BrokenPipeline::calibratedTempC10();
}
