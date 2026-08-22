// The hand-written extern "C" shim -- the realistic mechanism confirmed
// by research (bindgen can't bind templates; cxx needs an allocator,
// in tension with this ecosystem's no-heap design). Wraps ONE fully-
// instantiated, concrete HAPI-composed type behind flat functions and a
// static instance -- no heap, no exceptions, no RTTI, matching HAPI's
// own embedded conventions and this project's existing no-alloc stance.
//
// Same component shape as HAPI/examples/cuda_device_chain (Counter +
// DoubleStep composed via real hapi::APIOf<>) -- the SAME composition
// mechanism, now bridged to Rust instead of nvcc, for a direct
// side-by-side comparison across toolchains.
#include <hapi/hapi.h>

struct Counter {
  template<typename O> struct Part : O {
    using Base = O; using Base::Base;
    int value{0};
    void inc() { value += 1; Base::inc(); }
    int get() const { return value; }
  };
};

struct DoubleStep {
  template<typename O> struct Part : O {
    using Base = O; using Base::Base;
    void inc() { Base::inc(); Base::inc(); } // +2 per call
  };
};

struct CounterAPI {
  void inc() {}
};

// hapi::Chain<>'s real inheritance-fold + template-template-parameter
// pattern, unmodified -- exactly as used in cuda_device_chain.
using Ticker = hapi::APIOf<CounterAPI, DoubleStep, Counter>;

// Static storage, not heap -- Ticker's default ctor is trivial (just
// zero-inits `value`), so this needs no runtime constructor call, just
// .bss zero-init, matching this ecosystem's no-dynamic-allocation stance.
static Ticker g_ticker;

extern "C" {
  void hapi_ticker_inc() { g_ticker.inc(); }
  int  hapi_ticker_get() { return g_ticker.get(); }
}
