/**
 * @file reg.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi generic state-slot component.
*/

#pragma once

namespace hapi {
  /// @brief Chain-composable Part holding one value of type T, nothing
  /// else. A logic Part listed before it in a Chain reaches it via
  /// ordinary inheritance (Base::value).
  template<typename T>
  struct Reg {
    using Type = T;
    template<typename I>
    struct Part : I {
      using Base = I;
      using Base::Base;
      T value{};
    };
  };
}
