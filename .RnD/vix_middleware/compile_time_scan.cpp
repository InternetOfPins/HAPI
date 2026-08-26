// Compile-time cost of the Chain<>::Part approach at increasing
// middleware-chain depth -- the honest, disclosed tradeoff side, not
// just the runtime win. N synthetic middleware types (same shape as
// AuthCheck/RateLimit/LogPassthrough), depth set via -DDEPTH=<N>.
#include <hapi/hapi.h>
#include "vix_stand_ins.hpp"

using namespace vix_stub;

template<int I>
struct GenMW {
  template<typename O>
  struct Part : O {
    using Base = O;
    using Base::Base;
    void handle(Request &req, ResponseWrapper &res) {
      req.hits++;
      Base::handle(req, res);
    }
  };
};

struct Terminal {
  void handle(Request &, ResponseWrapper &res) { res.status = 200; }
};

#ifndef DEPTH
#define DEPTH 1
#endif

#if DEPTH >= 1
#define MW_1 GenMW<0>
#else
#define MW_1
#endif
#if DEPTH >= 2
#define MW_2 ,GenMW<1>
#else
#define MW_2
#endif
#if DEPTH >= 4
#define MW_4 ,GenMW<2>,GenMW<3>
#else
#define MW_4
#endif
#if DEPTH >= 8
#define MW_8 ,GenMW<4>,GenMW<5>,GenMW<6>,GenMW<7>
#else
#define MW_8
#endif

using Pipeline = hapi::Chain<MW_1 MW_2 MW_4 MW_8>::Part<Terminal>;

int main() {
  Request req;
  ResponseWrapper res;
  Pipeline p{};
  p.handle(req, res);
  return req.hits == DEPTH ? 0 : 1;
}
