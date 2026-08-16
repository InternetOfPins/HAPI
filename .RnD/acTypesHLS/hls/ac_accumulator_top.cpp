// Genuinely stateful ac_int example -- a running signed accumulator using
// Reg<Sum> for cross-call state, unlike ac_int_ops_top.cpp's purely
// combinational arithmetic. Deliberately narrow (8 bits) to also exercise
// wraparound through the shell.
#include <ac_int.h>
#include "../ac_upstream_guard.h"
#include <hapi/hapi.h>
#include <cstdint>
using namespace hapi;

using Sum = ac_int<8, true>;

struct AccumulateOp {
  template<typename I>
  struct Part : I {
    using Base = I;
    using Base::Base;

    Sum step(ac_int<8, true> x) {
      Base::value = Base::value + x;
      return Base::value;
    }
  };
};

using Accumulator = Chain<AccumulateOp, Reg<Sum>>;
using Top         = APIOf<Nil, Accumulator>;

Top acc;

int32_t acAccumulatorTop(int8_t x) {
  return acc.step(x).to_int();
}
