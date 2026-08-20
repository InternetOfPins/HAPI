// AVR footprint check for the corrected (int32_t-accumulator) A/B/C
// variants -- confirms the accumulator-width fix didn't cost the
// zero-overhead property, on the real target, not just natively.
#include <stdint.h>
#include <hapi/hapi.h>
using namespace hapi;

template<typename Accum, Accum... Ws> struct Neuron;
template<typename Accum> struct Neuron<Accum> { static constexpr Accum sum() { return 0; } };
template<typename Accum, Accum W0, Accum... Ws>
struct Neuron<Accum, W0, Ws...> {
  template<typename X0, typename... Xs>
  static Accum sum(X0 x0, Xs... xs) { return Accum(W0 * x0) + Neuron<Accum, Ws...>::sum(xs...); }
};

template<int16_t B0, int16_t B1, int16_t B2>
struct Normalize3 {
  template<typename O> struct Part : O {
    static auto forward(int16_t x0, int16_t x1, int16_t x2) {
      return O::forward(int16_t(x0-B0), int16_t(x1-B1), int16_t(x2-B2));
    }
  };
};

template<int16_t W00,int16_t W01,int16_t W02, int16_t W10,int16_t W11,int16_t W12>
struct Dense3to2 {
  template<typename O> struct Part : O {
    static auto forward(int16_t x0, int16_t x1, int16_t x2) {
      int16_t h0 = int16_t(Neuron<int32_t,W00,W01,W02>::sum(x0,x1,x2) >> 8);
      int16_t h1 = int16_t(Neuron<int32_t,W10,W11,W12>::sum(x0,x1,x2) >> 8);
      return O::forward(h0, h1);
    }
  };
};

struct Relu2 {
  template<typename O> struct Part : O {
    static auto forward(int16_t h0, int16_t h1) {
      return O::forward(h0 < 0 ? int16_t(0) : h0, h1 < 0 ? int16_t(0) : h1);
    }
  };
};

template<int16_t W00,int16_t W01, int16_t W10,int16_t W11, int16_t W20,int16_t W21>
struct Dense2to3 {
  template<typename O> struct Part : O {
    static auto forward(int16_t h0, int16_t h1) {
      int16_t o0 = int16_t(Neuron<int32_t,W00,W01>::sum(h0,h1) >> 8);
      int16_t o1 = int16_t(Neuron<int32_t,W10,W11>::sum(h0,h1) >> 8);
      int16_t o2 = int16_t(Neuron<int32_t,W20,W21>::sum(h0,h1) >> 8);
      return O::forward(o0, o1, o2);
    }
  };
};

struct ArgMax3 {
  template<typename O> struct Part : O {
    static uint8_t forward(int16_t o0, int16_t o1, int16_t o2) {
      uint8_t idx = 0; int16_t best = o0;
      if (o1 > best) { best = o1; idx = 1; }
      if (o2 > best) { best = o2; idx = 2; }
      return idx;
    }
  };
};

struct NetTerminal {};
using NormW = Normalize3<10,10,10>;
using D1    = Dense3to2<64,-32,16, -48,80,24>;
using D2    = Dense2to3<96,-40, 20,60, -30,50>;

using NetA = Chain<NormW, D1, Relu2, D2, ArgMax3>::Part<NetTerminal>;
using NetB = Chain<NormW, D1,        D2, ArgMax3>::Part<NetTerminal>;
using NetC = Chain<        D1, Relu2, D2, ArgMax3>::Part<NetTerminal>;

volatile int16_t v_x0 = 120, v_x1 = -15, v_x2 = 8;
volatile uint8_t g_out;

int main() {
#if VARIANT == 0
  g_out = NetA::forward(v_x0, v_x1, v_x2);
#elif VARIANT == 1
  g_out = NetB::forward(v_x0, v_x1, v_x2);
#else
  g_out = NetC::forward(v_x0, v_x1, v_x2);
#endif
  return 0;
}
