// PROOF OF CONCEPT ONLY -- experiment-2 branch (pushing past the
// documented "HAPI does not accelerate neural-network kernels" scope).
// Tests whether a Dense+Activation+Dense inference stack composes at
// zero overhead the same way Fir/Biquad/Pid's MAC chains do. Fixed
// 3-input/2-hidden/1-output shape, weights as compile-time NTTPs --
// deliberately NOT a generic N-input/M-output Dense template (that's the
// "arbitrary tensor shape" scope creep the HANDOFF explicitly avoids).
// Two variants built to test optional-elimination for THIS composition
// shape specifically, not just re-citing the existing GFX/blockchain proof.

#include <stdint.h>
#include <hapi/hapi.h>
using namespace hapi;

template<int16_t W00,int16_t W01,int16_t W02,
         int16_t W10,int16_t W11,int16_t W12>
struct Dense3to2 {
  template<typename O>
  struct Part : O {
    static int16_t forward(int16_t x0, int16_t x1, int16_t x2) {
      int16_t h0 = int16_t((W00*x0 + W01*x1 + W02*x2) >> 8);
      int16_t h1 = int16_t((W10*x0 + W11*x1 + W12*x2) >> 8);
      return O::forward(h0, h1);
    }
  };
};

struct Relu2 {
  template<typename O>
  struct Part : O {
    static int16_t forward(int16_t h0, int16_t h1) {
      return O::forward(h0 < 0 ? int16_t(0) : h0, h1 < 0 ? int16_t(0) : h1);
    }
  };
};

// At-inference no-op (Dropout is training-only) -- should vanish entirely.
struct DropoutNoop {
  template<typename O>
  struct Part : O {
    static int16_t forward(int16_t h0, int16_t h1) { return O::forward(h0, h1); }
  };
};

template<int16_t W0, int16_t W1>
struct Dense2to1 {
  template<typename O>
  struct Part : O {
    static int16_t forward(int16_t h0, int16_t h1) {
      return int16_t((W0*h0 + W1*h1) >> 8);
    }
  };
};

struct NetTerminal {};

// Hand-picked weights, not trained -- the point is the composition.
using Net = Chain<Dense3to2<64,-32,16, -48,80,24>, Relu2,
                  Dense2to1<96,-40>>::Part<NetTerminal>;

using NetWithDropout = Chain<Dense3to2<64,-32,16, -48,80,24>, Relu2, DropoutNoop,
                             Dense2to1<96,-40>>::Part<NetTerminal>;

// Inputs volatile -- weights are compile-time NTTPs (realistic: fixed
// after deployment), inputs are NOT (realistic: live sensor data). The
// first cut of this test left inputs as literals and the whole net
// constant-folded to one `ldi` -- real, but not the representative case.
// Same compile-time-vs-runtime distinction INDUSTRY.md's FIR section
// already tests for filter coefficients, applied here to inference input.
volatile int16_t v_x0 = 120, v_x1 = -15, v_x2 = 8;
volatile int16_t g_out;

#ifdef WITH_DROPOUT
int main() { g_out = NetWithDropout::forward(v_x0, v_x1, v_x2); return 0; }
#else
int main() { g_out = Net::forward(v_x0, v_x1, v_x2); return 0; }
#endif
