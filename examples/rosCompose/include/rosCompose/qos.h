/**
 * @file qos.h
 * @brief QoS as decorators over a subscriber Body.
 *
 * `WithHistory` (KEEP_LAST depth) and `WithDeadline` (missed-publish
 * detection) each fold a cross-cutting concern into a subscriber via the
 * *unchanged* `Subscriber<Msg,Body>` mechanism — same idiom as OneMenu's
 * item decorators. In real ROS these live in the RMW/DDS layer, not
 * rclcpp's C++; this is the local equivalent a DDS-less target needs.
 *
 * `WithDeadline` needs a millisecond clock; it uses OneChip's
 * `hw::Timeout<Ms>` / `hw::millis()`. `WithHistory` is standalone.
 */
#pragma once
#include <stdint.h>
#include "rosCompose/transport.h"

namespace rosCompose {

  /// Fixed-capacity KEEP_LAST ring. No heap. Wraparound is a conditional
  /// subtract, not `% N` (which lowers to a soft-division call on AVR).
  template<typename Msg, int N>
  struct History {
    Msg buf[N] = {};
    unsigned count = 0, head = 0;
    void push(const Msg& m) {
      unsigned i = head + count;
      if (i >= (unsigned)N) i -= (unsigned)N;
      buf[i] = m;
      if (count < (unsigned)N) ++count;
      else if (++head == (unsigned)N) head = 0;
    }
    const Msg& at(unsigned k) const {
      unsigned i = head + k;
      if (i >= (unsigned)N) i -= (unsigned)N;
      return buf[i];
    }
  };

  /// Subscriber-body decorator: retain a bounded history alongside on(m).
  template<typename Msg, int N, typename InnerBody>
  struct WithHistory {
    template<typename T>
    struct Part : InnerBody::template Part<T> {
      using Base = typename InnerBody::template Part<T>;
      using Base::Base;
      History<Msg, N> history;
      void on(const Msg& m) { history.push(m); Base::on(m); }
    };
  };

}

#if __has_include(<oneChip/clock.h>)
#include <oneChip/clock.h>
namespace rosCompose {

  /// Subscriber-body decorator: fire onMissed() once per episode if no
  /// message re-arms the latch within DeadlineMs.
  template<typename Msg, uint32_t DeadlineMs, typename InnerBody>
  struct WithDeadline {
    template<typename T>
    struct Part : InnerBody::template Part<T> {
      using Base = typename InnerBody::template Part<T>;
      using Base::Base;
      hw::Timeout<DeadlineMs> dl;
      bool missed = false;
      void (*onMissed)() = nullptr;
      void on(const Msg& m) { dl.reset(); missed = false; Base::on(m); }
      void checkDeadline() {
        if (!missed && dl) { missed = true; if (onMissed) onMissed(); }
      }
    };
  };

}
#endif
