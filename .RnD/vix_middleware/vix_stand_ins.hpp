// Minimal stand-ins for vix::http::Request/ResponseWrapper -- NOT vix's
// real types (no real vix::App reachable this round, see HANDOFF.md for
// why: two independent, unrelated CMake bugs in vix's own build system).
// Only what's needed to give Middleware/Next their real shape, confirmed
// exact from vix/modules/core/include/vix/app/App.hpp:106-111 this
// session:
//   using Next       = std::function<void()>;
//   using Middleware = std::function<void(Request&, ResponseWrapper&, Next)>;
#pragma once

#include <functional>
#include <string>

namespace vix_stub {

  struct Request {
    std::string path;
    int hits = 0;
  };

  struct ResponseWrapper {
    int status = 200;
  };

  using Next       = std::function<void()>;
  using Middleware = std::function<void(Request &, ResponseWrapper &, Next)>;

}
