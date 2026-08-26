// Opt-in call-site sketch, exactly the shape the outreach doc proposed:
//   app.get<Chain<Auth,RateLimit>>("/x", handler);
// Against a minimal stand-in App -- NOT real vix::App (unreachable this
// round, see HANDOFF.md). Proves the shape type-checks and dispatches
// correctly, not that it's wired into vix's real router.
#pragma once

#include "vix_stand_ins.hpp"
#include <hapi/hapi.h>
#include <string>
#include <utility>

namespace vix_stub {

  template<typename Handler>
  struct HandlerTerminal {
    Handler h;
    explicit HandlerTerminal(Handler h_) : h(std::move(h_)) {}
    void handle(Request &req, ResponseWrapper &res) { h(req, res); }
  };

  // Front-loaded shape check, same idiom as OneMenu's find<Q>() fix
  // (this session): reuses hapi::HasPart (rules.h) rather than a new
  // trait -- a middleware type with no nested Part<O> at all is the
  // realistic misuse (a data-only struct pasted in by mistake), and left
  // unchecked it used to cascade into ~40 lines of "no matching
  // constructor" candidate-overload noise once Chain<>::Part's own
  // instantiation failed downstream. This turns it into one line naming
  // the actual offending type.
  template<typename Input> struct AllMiddlewareShaped;
  template<> struct AllMiddlewareShaped<hapi::Chain<>> : std::true_type {};
  template<typename O, typename... OO>
  struct AllMiddlewareShaped<hapi::Chain<O, OO...>> : std::bool_constant<
    hapi::HasPart<O>::value && AllMiddlewareShaped<hapi::Chain<OO...>>::value> {};

  struct App {
    template<typename MWChain, typename Handler>
    void get(std::string path, Handler handler) {
      static_assert(AllMiddlewareShaped<typename MWChain::Types>::value,
        "app.get<Chain<...>>: every middleware type needs a nested "
        "Part<O> template (see AuthCheck/RateLimit/LogPassthrough for "
        "the shape) -- one of the types passed here doesn't have one");
      // The alias below (Pipeline = MWChain::Part<...>) instantiates
      // Chain<>::Part<T> immediately, regardless of branch -- a `using`
      // isn't lazy the way if constexpr's discarded statements are. So
      // it has to live INSIDE the taken branch, same reason find<Q>()'s
      // fix (menu.h, this session) gates its own failing call the same
      // way -- the static_assert above is necessary but not sufficient
      // on its own to suppress the downstream cascade.
      if constexpr (AllMiddlewareShaped<typename MWChain::Types>::value) {
        using Pipeline = typename MWChain::template Part<HandlerTerminal<Handler>>;
        // Every middleware layer's `using Base::Base;` (chain.h) propagates
        // HandlerTerminal's own constructor transparently -- Pipeline can be
        // built directly from the handler value, same as constructing the
        // terminal itself would be.
        Pipeline p{std::move(handler)};
        Request req{std::move(path), 0};
        ResponseWrapper res;
        p.handle(req, res);
        last_hits = req.hits;
        last_status = res.status;
      }
    }

    int last_hits = 0;
    int last_status = 0;
  };

}
