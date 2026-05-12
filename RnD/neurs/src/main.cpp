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

/// @brief a mini CRTP for a part
/// with this, 
/// @tparam Component : part component type 
template<typename Component>
struct HAPI {
  template<typename... XX> using Ins=typename hapi::Chain<XX...,Component>;
  template<typename... XX> using App=typename hapi::Chain<Component,XX...>;
  template<typename C> using Join=typename C::Ins<Component>;
  template<template<typename> class M> using Map=M<Component>;

  template<typename Comp,typename Ahead,typename Behind=TypeList<>>
  struct Rules {static constexpr const bool value{false};};
};

//the fall-back API code erasure
struct ItemAPI {
  static void api(const char*o) {cout<<o;}
};

template<typename... OO>
struct ItemDef : APIOf<ItemAPI, OO...> {
  using Base = APIOf<ItemAPI, OO...>;

  template<typename Comp>
  struct CheckRules {
    static constexpr bool value = AndAll<OO::template Rules<Comp>::check()...>::value;
  };

  static_assert(CheckRules<Base>::value, "HAPI Item: validation failed");

  using Base::Base;
};

struct Tag;
struct Bars;
struct Dots;

template<char oc, char cc=oc>
struct WrapWith:HAPI<WrapWith<oc,cc>> {
  template<typename I>
  struct Part:I {
    static void api(const char*o) {
      cout<<oc;
      I::api(o);
      cout<<cc;
    }
  };
  // Rules at feature level (cleaner)
  template<typename Comp>
  struct Rules {
    // using Comp=typename Before::Join<After>;
    static constexpr bool check() {
      static_assert(Same<Comp,class Tag>::value, "WrapWith likes to have a tag please.");
      static_assert(!Same<Comp,class Bars>::value, "WrapWith is allergic to bars!");
      static_assert(!Same<Comp,class Dots>::value, "no dots please");
      return true;
    }
  };
};

struct Parens  :WrapWith<'(',')'>{};
struct SqBracks:WrapWith<'[',']'>{};
struct Bracks  :WrapWith<'{','}'>{};
struct Bars    :WrapWith<'|'>{};
struct Tag     :WrapWith<'<','>'>{};
struct Dots    :WrapWith<'.'>{};

ItemDef<Tag,SqBracks,Bracks,Parens,WrapWith<'_'>> testItem;

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
    testItem.api("*");
    cout<<endl;
    return 0;
  }
#endif
