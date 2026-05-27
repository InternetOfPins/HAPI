/**
 * @file hapi.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief 
 * 
*/

#ifdef __AVR__
  #include "platform/avr/avr_std.h"
  using size_t=unsigned int;
#else
  #include <cstddef>
  #include <type_traits>
  #include <utility>
#endif

#ifdef HAPI_DEBUG
  #include <iostream>
  using std::cout;
  using std::endl;
  namespace hapi{};
#endif

#ifndef HAPI_DEBUG
  namespace hapi {
#endif
  #include "hapi/rules.h"
  #include "hapi/chain.h"

  struct Nil {};
#ifndef HAPI_DEBUG
  };//namespace hapi
#endif
