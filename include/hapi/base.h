#pragma once

#ifdef __AVR__
  #include "platform/avr/avr_std.h"
#elif defined(__XTENSA__) 
  #include "platform/xtensa/xtensa_std.h"
#else
  #include <type_traits>
  #include <utility>
  #include <stddef.h>
  using std::size_t;
#endif

