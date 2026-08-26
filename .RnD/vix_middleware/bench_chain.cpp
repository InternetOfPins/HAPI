// Chain<>::Part equivalent of bench_baseline.cpp -- same 3 middleware
// semantics (Auth, RateLimit, Log), dispatched ITERS times, for
// disassembly/binary-size comparison. Direct calls, no std::function,
// no per-request closure -- the whole point being measured.
#include "chain_middleware.hpp"
#include <cstdlib>

using namespace vix_stub;
using namespace hapi;

static volatile int sink = 0;

int main(int argc, char **argv) {
  int iters = argc > 1 ? std::atoi(argv[1]) : 100000;

  using Pipeline = MiddlewarePipeline<AuthCheck, RateLimit, LogPassthrough>;

  for (int i = 0; i < iters; i++) {
    Request req;
    ResponseWrapper res;
    Pipeline p{};
    p.handle(req, res);
    sink += req.hits;
  }
  return sink != iters * 3;
}
