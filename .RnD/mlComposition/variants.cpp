// Gaps 3 + 4 from the reviewer's feedback:
//   (3) A/B/C structural variants, checked for CORRECT output vs a boring
//       baseline -- not just footprint.
//   (4) Weighted-sum factored out as its own reusable primitive (Neuron<>,
//       the Tap<> analog for Dense) instead of hand-fused into each layer.
// Native build -- pure header logic, no AVR register access, safe to run
// directly and compare against a boring-baseline reference.

#include <cstdint>
#include <cstdio>
#include <cassert>
#include <hapi/hapi.h>
using namespace hapi;

// ---- Neuron<Accum, W...>: reusable weighted-sum primitive --------------
// Recursive peel of the weight pack against the input pack, one term per
// specialization -- same shape as Fir<>'s per-tap Chain, generalized from
// "1 growing accumulator over a delay line" to "1 accumulator over M
// distinct simultaneous inputs." This is the piece Stage-2's first PoC
// hand-inlined instead of factoring out.
template<typename Accum, Accum... Ws> struct Neuron;

template<typename Accum>
struct Neuron<Accum> { static constexpr Accum sum() { return 0; } };

template<typename Accum, Accum W0, Accum... Ws>
struct Neuron<Accum, W0, Ws...> {
  template<typename X0, typename... Xs>
  static Accum sum(X0 x0, Xs... xs) {
    return Accum(W0 * x0) + Neuron<Accum, Ws...>::sum(xs...);
  }
};

// ---- Pipeline components, each independently defined -------------------
template<int16_t B0, int16_t B1, int16_t B2>
struct Normalize3 {
  template<typename O> struct Part : O {
    static auto forward(int16_t x0, int16_t x1, int16_t x2) {
      return O::forward(int16_t(x0-B0), int16_t(x1-B1), int16_t(x2-B2));
    }
  };
};

template<int16_t W00,int16_t W01,int16_t W02,
         int16_t W10,int16_t W11,int16_t W12>
struct Dense3to2 {
  template<typename O> struct Part : O {
    static auto forward(int16_t x0, int16_t x1, int16_t x2) {
      // Accum is int32_t, wider than the int16_t weights/inputs -- same
      // Sample/Accum split oneHLS's Fir/Biquad/Pid already use, and for
      // the same reason: summing narrow terms in a narrow accumulator
      // overflows. First cut of this file used int16_t here and a real
      // input (300,-300,0) silently produced a wrong ArgMax class --
      // caught by the boring-baseline comparison below, not by inspection.
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

// ---- ArgMax: new decision stage -----------------------------------------
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

// Same weights reused across all three variants -- only the Chain<> list
// changes. This is the actual test: does structural variation map onto
// composition-level swaps, not reimplementation.
using NormW = Normalize3<10,10,10>;
using D1    = Dense3to2<64,-32,16, -48,80,24>;
using D2    = Dense2to3<96,-40, 20,60, -30,50>;

using hapi_A_t = ArgMax3;

using NetA = Chain<NormW, D1, Relu2, D2, ArgMax3>::Part<NetTerminal>; // full
using NetB = Chain<NormW, D1,        D2, ArgMax3>::Part<NetTerminal>; // no ReLU
using NetC = Chain<        D1, Relu2, D2, ArgMax3>::Part<NetTerminal>; // no Normalize

// ---- Boring baseline: ordinary C++, no composition, same arithmetic ----
static uint8_t boringA(int16_t x0,int16_t x1,int16_t x2) {
  x0-=10; x1-=10; x2-=10;
  int16_t h0 = int16_t((64*x0 + -32*x1 + 16*x2) >> 8);
  int16_t h1 = int16_t((-48*x0 + 80*x1 + 24*x2) >> 8);
  if (h0<0) h0=0; if (h1<0) h1=0;
  int16_t o0 = int16_t((96*h0 + -40*h1) >> 8);
  int16_t o1 = int16_t((20*h0 + 60*h1) >> 8);
  int16_t o2 = int16_t((-30*h0 + 50*h1) >> 8);
  uint8_t idx=0; int16_t best=o0;
  if (o1>best){best=o1;idx=1;} if (o2>best){best=o2;idx=2;}
  return idx;
}
static uint8_t boringB(int16_t x0,int16_t x1,int16_t x2) {
  x0-=10; x1-=10; x2-=10;
  int16_t h0 = int16_t((64*x0 + -32*x1 + 16*x2) >> 8);
  int16_t h1 = int16_t((-48*x0 + 80*x1 + 24*x2) >> 8);
  // no ReLU
  int16_t o0 = int16_t((96*h0 + -40*h1) >> 8);
  int16_t o1 = int16_t((20*h0 + 60*h1) >> 8);
  int16_t o2 = int16_t((-30*h0 + 50*h1) >> 8);
  uint8_t idx=0; int16_t best=o0;
  if (o1>best){best=o1;idx=1;} if (o2>best){best=o2;idx=2;}
  return idx;
}
static uint8_t boringC(int16_t x0,int16_t x1,int16_t x2) {
  // no Normalize -- raw x0,x1,x2
  int16_t h0 = int16_t((64*x0 + -32*x1 + 16*x2) >> 8);
  int16_t h1 = int16_t((-48*x0 + 80*x1 + 24*x2) >> 8);
  if (h0<0) h0=0; if (h1<0) h1=0;
  int16_t o0 = int16_t((96*h0 + -40*h1) >> 8);
  int16_t o1 = int16_t((20*h0 + 60*h1) >> 8);
  int16_t o2 = int16_t((-30*h0 + 50*h1) >> 8);
  uint8_t idx=0; int16_t best=o0;
  if (o1>best){best=o1;idx=1;} if (o2>best){best=o2;idx=2;}
  return idx;
}

int main() {
  int fails = 0;
  int16_t tests[][3] = {{120,-15,8},{0,0,0},{-50,200,30},{10,10,10},{300,-300,0}};
  for (auto& t : tests) {
    uint8_t a = NetA::forward(t[0],t[1],t[2]), ba = boringA(t[0],t[1],t[2]);
    uint8_t b = NetB::forward(t[0],t[1],t[2]), bb = boringB(t[0],t[1],t[2]);
    uint8_t c = NetC::forward(t[0],t[1],t[2]), bc = boringC(t[0],t[1],t[2]);
    printf("in(%4d,%4d,%4d)  A: composed=%u boring=%u %s | B: composed=%u boring=%u %s | C: composed=%u boring=%u %s\n",
      t[0],t[1],t[2], a,ba,(a==ba?"OK":"FAIL"), b,bb,(b==bb?"OK":"FAIL"), c,bc,(c==bc?"OK":"FAIL"));
    if (a!=ba) fails++; if (b!=bb) fails++; if (c!=bc) fails++;
  }
  printf(fails==0 ? "\nALL VARIANTS MATCH BORING BASELINE\n" : "\n%d MISMATCHES\n", fails);
  return fails;
}
