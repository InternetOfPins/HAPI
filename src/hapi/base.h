#pragma once

#ifdef __AVR__
  #include "hapi/platform/avr/avr_std.h"
  using avr_std::enable_if;
  using avr_std::true_type;
  using avr_std::false_type;
  using avr_std::declval;
  using avr_std::is_class;
  using avr_std::forward;
  using avr_std::remove_reference;
#else
  #include <type_traits>
  using std::enable_if;
  using std::true_type;
  using std::false_type;
  using std::declval;
  using std::is_class;
  using std::forward;
  using std::remove_reference;
#endif

template<bool chk,typename T=void> using When=typename enable_if<chk,T>::type;

