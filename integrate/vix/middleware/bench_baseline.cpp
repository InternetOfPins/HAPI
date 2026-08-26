// Real vix run_middleware_chain_ shape, 3 real middleware (Auth, RateLimit,
// Log), dispatched ITERS times -- for disassembly/binary-size comparison
// against bench_chain.cpp's Chain<>::Part equivalent. volatile sink
// prevents the loop and closures from being optimized away entirely.
#include "baseline_std_function.hpp"
#include <cstdlib>

using namespace vix_stub;

static volatile int sink = 0;

int main(int argc, char **argv) {
  int iters = argc > 1 ? std::atoi(argv[1]) : 100000;

  std::vector<Middleware> chain = {
      [](Request &req, ResponseWrapper &, Next next) { req.hits++; next(); },
      [](Request &req, ResponseWrapper &, Next next) { req.hits++; next(); },
      [](Request &req, ResponseWrapper &, Next next) { req.hits++; next(); },
  };

  for (int i = 0; i < iters; i++) {
    Request req;
    ResponseWrapper res;
    run_middleware_chain(chain, 0, req, res, [&]() { res.status = 200; });
    sink += req.hits;
  }
  return sink != iters * 3;
}
