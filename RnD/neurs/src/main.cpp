/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief flat modular API, static and non-recursive API interface
 * @version 1
 * @date 2025-04-10
 * 
 */

#include <hapi.h>
using namespace hapi;

struct ItemAPI {
  void put() {cout<<'*';}
};

struct Rules {static constexpr const bool value{false};};

struct Item {
  template<typename Target,typename Ahead,typename Behind=Chain<>>
  static constexpr const bool check() {return true;}

  template<typename Ahead,typename Behind=Chain<>>
  static constexpr const std::enable_if_t<Ahead::size==0,bool> _check() {return false;}

  template<typename Ahead,typename Behind=Chain<>>
  static constexpr const std::enable_if_t<Ahead::size!=0,bool> _check() {
    return Ahead::Head::template Check<typename Ahead::Head,typename Ahead::Tail,Behind>::value
      || Item::_check<typename Ahead::Tail,typename Behind::template Append<typename Ahead::Head>>();
  }

  template<typename Target, typename Ahead=Chain<>,typename Behind=Chain<>> using Check=Rules;
};

template<typename... OO>
struct ItemDef:APIOf<ItemAPI,OO...> {
  using Base=APIOf<ItemAPI,OO...>;

  static constexpr const bool check() {
    return Chain<OO...>::Head::template _check<Chain<OO...>>();
  }

  static_assert((check(),true),"fail!");
};

class Yawn;
class Zzz;
class Snore;
class Wtf;

struct Zzz:Item {
  template<typename O>
  struct Part:O {
    using Base=O;
  };

  template<typename Target, typename Ahead,typename Behind=Chain<>>
  struct Check:Rules {
    static_assert(!Has<IsSame<class Snore>>::Before::template check<Behind,Ahead>(),"No Snore before Zzz");
    static_assert(!Has<IsSame<Yawn>>::After::template check<Behind,Ahead>(),"No Yawn after Zzz");
  };

};

struct Yawn:Item {
  template<typename O>
  struct Part:O {
    using Base=O;
  };
};

struct Snore:Item {

  template<typename O>
  struct Part:O {
    using Base=O;
    using This=Part<O>;
  };

  template<typename Target, typename Ahead=Chain<>,typename Behind=Chain<>>
  struct Check:Rules {
    static_assert(Has<IsSame<Zzz>>::Before::template check<Behind,Ahead>(),"Zzz before Snore");
    static_assert(!Has<IsSame<Zzz>>::After::template check<Behind,Ahead>(),"No Zzz after Snore");
  };

};

struct Wtf:Item {
  template<typename O>
  struct Part:O {
    using Base=O;
    using This=Part<O>;
  };
};

ItemDef<Yawn,Zzz,Snore> testItem;

#ifdef ARDUINO
  void setup() {
    Serial.begin(115200);
    while(!Serial);
  }
  void loop() {
    testItem.put();
    cout<<endl;
    delay(1000);
  }
#else
  int main() {
    cout<<testItem.check()<<endl;
    cout<<endl;
    return 0;
  }
#endif
 