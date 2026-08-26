// Baseline: reproduces vix's real App.cpp run_middleware_chain_ shape
// exactly (confirmed this session, App.cpp:829-845-ish) -- a recursive
// walk over std::vector<Middleware>, allocating a fresh Next
// (std::function<void()>) closure per layer per call. This is the real,
// current cost this opportunity targets, not an approximation of it.
#pragma once

#include "vix_stand_ins.hpp"
#include <vector>

namespace vix_stub {

  inline void run_middleware_chain(
      const std::vector<Middleware> &chain,
      std::size_t i,
      Request &req,
      ResponseWrapper &res,
      std::function<void()> final_handler) {
    if (i >= chain.size()) {
      final_handler();
      return;
    }
    auto next = [&]() {
      run_middleware_chain(chain, i + 1, req, res, std::move(final_handler));
    };
    chain[i](req, res, Next(next));
  }

}
