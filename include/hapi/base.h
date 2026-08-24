/**
 * @file base.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief HAPI base definitions
*/

#pragma once

// __AVR__ toolchains, and some bare-metal newlib/picolibc cross toolchains
// (e.g. riscv64-unknown-elf-g++ as packaged for rv32e/CH32V003) ship a C
// library but no C++ libstdc++ port -- <cstddef>/<type_traits>/<utility>
// don't exist there at all. Detect via __has_include instead of hardcoding
// __AVR__ as the only such platform.
#if defined(__has_include)
  #define HAPI_HAS_STL_HEADERS __has_include(<cstddef>)
#else
  #define HAPI_HAS_STL_HEADERS 1
#endif

#if defined(__AVR__) || !HAPI_HAS_STL_HEADERS
  #include "platform/avr/avr_std.h"
  namespace hapi { using SizeT=__SIZE_TYPE__; }
#else
  #include <cstddef>
  #include <type_traits>
  #include <utility>
  namespace hapi { using SizeT=size_t; }
#endif

#ifdef HAPI_DEBUG
  #include <iostream>
  using std::cout;
  using std::endl;
  namespace hapi{};
#endif

