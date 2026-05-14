/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief flat modular API, static and non-recursive API interface
 * @version 1
 * @date 2025-04-10
 * 
 */
#ifdef ARDUINO
  #include <Arduino.h>
#endif

#ifdef __AVR__
  #include "streamFlow.h"
  using namespace StreamFlow;
  #define endl "\n"
  #define cout Serial
#else
  #include <iostream>
  using namespace std;
#endif

#include <hapi.h>
using namespace hapi;

struct ItemAPI {
  void put() {cout<<'*';}
};

struct Item {
  template<typename Target,typename Ahead,typename Behind=Chain<>>
  static constexpr const bool check() {return true;}
  template<typename Ahead,typename Behind=Chain<>>
  static constexpr const std::enable_if_t<Ahead::size==0,bool> _check() {return true;}
  template<typename Ahead,typename Behind=Chain<>>
  static constexpr const std::enable_if_t<Ahead::size!=0,bool> _check() {
    return Ahead::Head::template check<typename Ahead::Head,typename Ahead::Tail,Behind>
      || Item::_check<typename Ahead::Tail,typename Behind::Append<typename Ahead::Head>>;
  }
};

template<typename... OO>
struct ItemDef:APIOf<ItemAPI,OO...> {
  using Base=APIOf<ItemAPI,OO...>;

  //Ahead and Behind can only come from here, start the members walk here
  static constexpr const bool check() {
    return Chain<OO...>::Head::template _check<Chain<OO...>>();
  }

  static_assert(check(),"fail!");
};

struct Zzz:Item {
  template<typename O>
  struct Part:O {
    using Base=O;
  };
  template<typename Target, typename Ahead=Chain<>,typename Behind=Chain<>>
  static constexpr const bool check() {
    static_assert(!Has<IsSame<class Snore>>::Before::template check<Ahead,Behind>(),"No snore before Zzz");
    return true;
  }
};

struct Yawn:Item {
  template<typename O>
  struct Part:O {
    using Base=O;

  };
  template<typename Target, typename Ahead=Chain<>,typename Behind=Chain<>>
  static constexpr const bool check() {
    static_assert(!Has<IsSame<Zzz>>::After::template check<Ahead,Behind>(),"No yawns after Zzz");
    return true;
  }
};

struct Snore:Item {
  template<typename O>
  struct Part:O {
    using Base=O;
    using This=Part<O>;
    template<typename Target, typename Ahead=Chain<>,typename Behind=Chain<>>
    static constexpr const bool check() {
      static_assert(Has<IsSame<Zzz>>::Before::template check<Ahead,Behind>(),"Zzz before Snore");
      return true;
    }
  };
};

ItemDef<Zzz,Zzz,Snore> ok;

#ifdef ARDUINO
  void setup() {
    Serial.begin(115200);
    while(!Serial);
  }
  void loop() {
    testItem.api("ok");
    cout<<endl;
    delay(1000);
  }
#else
  int main() {
    cout<<endl;
    return 0;
  }
#endif
 