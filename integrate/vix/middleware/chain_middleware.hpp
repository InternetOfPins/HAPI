// Opt-in Chain<>::Part middleware pipeline -- same semantic shape as
// App::use()'s handlers (auth check, rate limit, logging pass-through),
// composed via direct inheritance instead of a std::vector<std::function>
// walked at request time. Each layer calls Base::handle(...) directly --
// a compile-time-resolved call, not an indirect std::function invocation,
// and no Next closure is ever constructed.
#pragma once

#include "vix_stand_ins.hpp"
#include <hapi/hapi.h>

namespace vix_stub {

  // -- stub middleware, same semantic role as real App::use() handlers --

  struct AuthCheck {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;
      void handle(Request &req, ResponseWrapper &res) {
        req.hits++;              // stand-in for "validated" work
        Base::handle(req, res);
      }
    };
  };

  struct RateLimit {
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

  struct LogPassthrough {
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

  // -- terminal: the actual route handler, innermost layer --

  struct RouteTerminal {
    void handle(Request &, ResponseWrapper &res) {
      res.status = 200;
    }
  };

  template<typename... MW>
  using MiddlewarePipeline = typename hapi::Chain<MW...>::template Part<RouteTerminal>;

}
