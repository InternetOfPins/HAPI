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

template<typename O,typename... OO>
struct ItemDef:APIOf<ItemAPI,O,OO...> {
  using Base=APIOf<ItemAPI,O,OO...>;
  template<template<typename...> class Predicate,typename Comp>
  bool query() {return Predicate<O,Base>::value||(Query<Predicate,OO,Base>::value||...);}
  static constexpr bool check(){
    static_assert(query<IsSame,Base>(),"Wtf!");
    return true;
  };
  static_assert(check(),"fail");
};

struct Yawn {
  template<typename O>
  struct Part:O {
    using Base=O;
  };
  // template<typename Ahead,typename Behind=hapi::Chain<>>
};

struct Zzz {
  template<typename O>
  struct Part:O {
    using Base=O;
  };
};

struct Snore {
  template<typename O>
  struct Part:O {
    using Base=O;
    using This=Part<O>;
  };
};

Chain<Yawn,Yawn>::Append<Zzz> ok;

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
    cout<<IsSame<Zzz>::value<Zzz,Chain<>><<endl;
    cout<<ok.size<<endl;
    cout<<ok.Has<Snore><<endl;
    cout<<endl;
    return 0;
  }
#endif
 