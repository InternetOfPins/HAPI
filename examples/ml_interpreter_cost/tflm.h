// The same network run through TFLite-Micro's MicroInterpreter: a flatbuffer
// model blob (models/hello_world_*_model.h), a fixed tensor arena, a
// MicroMutableOpResolver, and MicroInterpreter::Invoke -- which dispatches
// each operator through a function pointer read from a runtime data
// structure. -DTFLM_INT8 selects the int8-quantised model.
#pragma once
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#ifdef TFLM_INT8
#include "models/hello_world_int8_model.h"
#define TFLM_MODEL_BLOB hello_world_int8_tflite
#else
#include "models/hello_world_float_model.h"
#define TFLM_MODEL_BLOB hello_world_float_tflite
#endif

namespace tflm {

// hello_world needs ~2.6 KB of arena; 4 KB is a round number with headroom.
constexpr int kArenaSize = 4096;
alignas(16) inline uint8_t g_arena[kArenaSize];

inline tflite::MicroInterpreter* g_interp = nullptr;

inline void setup() {
  const tflite::Model* model = tflite::GetModel(TFLM_MODEL_BLOB);
  static tflite::MicroMutableOpResolver<1> resolver;
  resolver.AddFullyConnected();
  static tflite::MicroInterpreter interp(model, resolver, g_arena, kArenaSize);
  g_interp = &interp;
  g_interp->AllocateTensors();
}

inline float infer(float x) {
  TfLiteTensor* in = g_interp->input(0);
  TfLiteTensor* out = g_interp->output(0);
#ifdef TFLM_INT8
  in->data.int8[0] = (int8_t)(x / in->params.scale + in->params.zero_point + 0.5f);
  g_interp->Invoke();
  return (out->data.int8[0] - out->params.zero_point) * out->params.scale;
#else
  in->data.f[0] = x;
  g_interp->Invoke();
  return out->data.f[0];
#endif
}

} // namespace tflm
