// Sanity check: does HAPI's own composition mechanism (Chain<>/APIOf<>,
// no chip-specific I/O) compile zero-overhead on real Xtensa (ESP32),
// same as it does on AVR and ARM? No SDK needed for this part -- pure
// template metaprogramming.
#include <stdint.h>
#include <hapi/hapi.h>
using namespace hapi;

template<int16_t W> struct Weighted {
  template<typename O> struct Part : O {
    static int16_t forward(int16_t x) { return O::forward(int16_t(W*x)); }
  };
};
struct Term { static int16_t forward(int16_t x) { return x; } };
using Chained = Chain<Weighted<3>, Weighted<7>, Weighted<11>>::Part<Term>;

volatile int16_t v_in = 42;
volatile int16_t v_out;
extern "C" void app_main() { v_out = Chained::forward(v_in); }
