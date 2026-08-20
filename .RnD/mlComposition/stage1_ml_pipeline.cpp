// Verifies HAPI/docs/INDUSTRY.md's existing "Edge AI & TinyML" claim for
// real: Sensor -> Calibration -> Filtering -> Feature Extraction ->
// Quantisation -> Inference Input, composed on real AVR hardware, using
// only existing IOP machinery plus one small new Part (Calibrate).
//
// Deliberately stops at the documented boundary: "HAPI does not accelerate
// neural-network kernels or execution engines." Inference Input is an
// explicit stub -- everything before it is real.

#include <hapi/hapi.h>
#include <oneData/oneData.h>
#include <oneHLS/oneHLS.h>
#include <chips/avr/avrDevice.h>
#include <oneIO/sensor/aht.h>

using namespace hapi;

using I2c    = hw::avr::mega::Twi<>;
using Sensor = oneIO::sensor::AHT<I2c>;

// ---- Calibration: new, chain-splice-able Part ---------------------------
// Fixed per-unit offset correction (e.g. factory trim). No existing IOP
// "Calibrate" component to reuse -- this is genuinely new, kept minimal.
// Listed FIRST in the Chain below so it can reach Sensor's tempC10() via
// O:: (first-listed = outermost = can reach everything after it).
template<int16_t OffsetX10>
struct Calibrate {
  template<typename O>
  struct Part : O {
    static int16_t calibratedTempC10() { return int16_t(O::tempC10() + OffsetX10); }
  };
};

struct PipelineTerminal { static void begin() {} };

using Pipeline = Chain<Calibrate<-3>, Sensor>::Part<PipelineTerminal>; // -0.3C factory offset

// ---- Filtering: real reuse -- oneHLS::Biquad as a plain member ---------
// Same idiom as the IoT demo's Pid: closed APIOf type, held as a member,
// .step() called per sample. Plain-int coefficients (MCU leg, not HLS).
static oneHLS::Biquad<int16_t, int32_t, 64, 64, 32, 8> lowpass;

// ---- Quantisation: same "raw bits, not a converting cast" discipline ---
// oneHLS's rawCoeff/RawBitsCtor already establishes for coefficients,
// applied here to a runtime value instead of a compile-time one.
static int8_t quantise(int16_t tempX10) {
  int16_t clipped = tempX10 < 0 ? 0 : (tempX10 > 635 ? 635 : tempX10); // 0..63.5C -> int8 @ /5
  return int8_t(clipped / 5);
}

// ---- Inference Input: explicit stub, marking INDUSTRY.md's own scope ---
static int8_t g_lastInferenceInput = 0;
static void inferenceInput(int8_t q) { g_lastInferenceInput = q; } // TODO: real model call

int main() {
  Pipeline::begin();
  Pipeline::measure();                                  // Sensor
  int16_t calibrated = Pipeline::calibratedTempC10();   // Calibration
  int16_t filtered   = int16_t(lowpass.step(calibrated)); // Filtering
  int8_t  feature    = int8_t(filtered);                // Feature Extraction (trivial here)
  int8_t  quantised  = quantise(feature);                // Quantisation
  inferenceInput(quantised);                             // Inference Input (stub)
  return 0;
}

static_assert(sizeof(Pipeline) <= 4, "Pipeline composition grew unexpectedly");
