// The same fully-connected network as models/hello_world_float.tflite
// (FC 1->16 +ReLU, FC 16->16 +ReLU, FC 16->1), built as a HAPI Chain<> of
// layer components. Weights are the trained values from that .tflite,
// extracted by tools/extract_weights.py into hello_world_weights.h as
// constexpr float arrays -- so this and TFLite-Micro compute the identical
// function, and the only thing that differs is how.
//
// Each layer is a Part<O>: it computes its output and hands it to O (the
// next layer, then the terminal). The Chain<> collapses to one flat
// function -- no per-layer dispatch, no interpreter, no tensor arena.
#pragma once
#include <hapi/hapi.h>
#include "hello_world_weights.h"

namespace net_a {
using namespace hapi;

// y = W.x + b ; W is [OUT][IN] row-major. W and b are references to the
// constexpr weight arrays (C++17 allows reference non-type template
// parameters with no linkage requirement).
template<int IN, int OUT, const float (&W)[IN * OUT], const float (&B)[OUT]>
struct Fc {
  template<typename O> struct Part : O {
    static void forward(const float* x) {
      float y[OUT];
      for (int o = 0; o < OUT; ++o) {
        float acc = B[o];
        const float* w = &W[o * IN];
        for (int i = 0; i < IN; ++i) acc += w[i] * x[i];
        y[o] = acc;
      }
      O::forward(y);
    }
  };
};

template<int N>
struct Relu {
  template<typename O> struct Part : O {
    static void forward(const float* x) {
      float y[N];
      for (int i = 0; i < N; ++i) y[i] = x[i] < 0.f ? 0.f : x[i];
      O::forward(y);
    }
  };
};

struct Sink {
  static float value;
  static void forward(const float* y) { value = y[0]; }
};
inline float Sink::value = 0.f;

using Net = Chain<
    Fc<1, 16, HW_W0, HW_B0>,  Relu<16>,
    Fc<16, 16, HW_W1, HW_B1>, Relu<16>,
    Fc<16, 1, HW_W2, HW_B2>
  >::Part<Sink>;

inline float infer(float x) { Net::forward(&x); return Sink::value; }

} // namespace net_a
