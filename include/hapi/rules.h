/**
 * @file rules.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief rules system for HAPI
 * @contributor Grok (xAI) - architecture, cleanup & modern C++ patterns
*/

#pragma once

#include "hapi/api.h"

#ifdef __AVR__
  #include "platform/avr/avr_std.h"
#elif defined(__XTENSA__) 
  #include "platform/xtensa/xtensa_std.h"
#else
  #include <type_traits>
#endif

namespace hapi {
  template<typename Crit>
  struct IsSame {
    template<typename... OO>
    static constexpr const bool check() {return (std::is_same_v<Crit,OO>||...);}
  };

  template<typename Predicate>
  struct Has {
    template<typename Ahead,typename Behind=Chain<>>
    static constexpr const std::enable_if_t<Ahead::size==0,bool> check() {return false;}
    
    template<typename Ahead,typename Behind=Chain<>>
    static constexpr const std::enable_if_t<Ahead::size!=0,bool> check() 
      {return Ahead::template Join<Behind>::template require<Predicate>;}

    struct Before {
      // template<typename Ahead,typename Behind=hapi::Chain<>>
      // static constexpr const std::enable_if_t<Ahead::size==0,bool> check() {return false;}

      template<typename Ahead,typename Behind=Chain<>>
      static constexpr const bool check()
        {return Ahead::template require<Predicate>;}
    };

    struct After {
      // template<typename Ahead,typename Behind=hapi::Chain<>>
      // static constexpr const std::enable_if_t<Ahead::size==0,bool> check() {return false;}

      template<typename Ahead,typename Behind=Chain<>>
      static constexpr const bool check()
        {return Behind::template require<Predicate>;}
    };
  };
};//namespace hapi