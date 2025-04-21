#pragma once

#ifdef ARDUINO
  #include <Arduino.h>
#endif

#ifdef __AVR__
  #include <streamFlow.h>
  using namespace StreamFlow;
  #define cout Serial
  #define endl "\n"
#else
  #include <iostream>
  using std::cout;
  using std::endl;
#endif

#ifdef __AVR__
  #include <assert.h>
  #include "hapi/platform/avr/avr_std.h"
  using avr_std::enable_if;
  using avr_std::true_type;
  using avr_std::false_type;
  using avr_std::declval;
  using avr_std::is_class;
  using avr_std::forward;
  using avr_std::move;
  using avr_std::remove_reference;
  #else
  #include <cassert>
  #include <type_traits>
  using std::enable_if;
  using std::true_type;
  using std::false_type;
  using std::declval;
  using std::is_class;
  using std::forward;
  using std::move;
  using std::remove_reference;
  using std::operator<<;
#endif

template<bool chk,typename T=void> using When=typename enable_if<chk,T>::type;

#define cex constexpr

/// @brief force constexpr max
/// @tparam T type of comparing values
/// @tparam a first value
/// @tparam b second value
/// @return the larger of the value
template<typename T,T a, T b> constexpr T cexMax() {return a>b?a:b;}

