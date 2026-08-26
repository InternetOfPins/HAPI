// Proves the doc's proposed opt-in call-site shape compiles and
// dispatches correctly: app.get<Chain<Auth,RateLimit>>("/x", handler).
#include "chain_middleware.hpp"
#include "stub_app.hpp"
#include <cassert>
#include <iostream>

using namespace vix_stub;
using namespace hapi;

int main() {
  App app;
  bool handlerRan = false;

  app.get<Chain<AuthCheck, RateLimit, LogPassthrough>>(
      "/x", [&](Request &req, ResponseWrapper &res) {
        handlerRan = true;
        res.status = 201;
        (void)req;
      });

  assert(handlerRan);
  assert(app.last_status == 201);
  assert(app.last_hits == 3);  // AuthCheck + RateLimit + LogPassthrough each ++hits

  std::cout << "api_sketch_demo: OK (hits=" << app.last_hits
            << " status=" << app.last_status << ")\n";
  return 0;
}
