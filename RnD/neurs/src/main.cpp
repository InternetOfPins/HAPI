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
};

//the fall-back API code erasure
struct ItemAPI {
  static void api(const char*o) {cout<<o;}
};

// Core Has<> - lightweight
// template<typename Comp, typename Tag>
// struct Has {
//   static constexpr bool value = false;
// };

// Specialization for your compositions
template<typename Part,typename API, typename... Fs, typename Tag>
struct Has {//<APIOf<API, Fs...>, Tag> {
  static constexpr bool value = !AndAll<
    !std::is_same_v<Part, Tag>...
  >::value;   // adjust marker name if needed
};

template<typename... OO>
struct ItemDef:APIOf<ItemAPI,OO...> {
  using Base=APIOf<ItemAPI,OO...>;

  template<typename Comp>
  struct CheckRules {
    static constexpr bool value = AndAll<
      OO::template Rules<Comp>::check()...
    >::value;
  };

  static_assert(CheckRules<Base>::value, "HAPI Item: validation failed");
};

// template<typename... OO>
// struct CheckRules<ItemDef<OO...>> {
//   using Base=ItemDef<OO...>;
//   static constexpr bool value = AndAll<
//     OO::template Rules<Base>::check()...
//   >::value;
// };

struct Parens;
struct SqBracks;
struct Bracks;
struct Bars;
struct Tag;

template<char oc, char cc>
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
  template<typename Part,typename Comp>
  struct Rules {
    static constexpr bool check() {
      static_assert(Has<Part,Comp,Tag>::value, "WrapWith likes to have a tag please.");
      static_assert(!Has<Part,Comp,Bars>::value,    "WrapWith is allergic to bars");
      return true;
    }
  };
};

struct Parens  :WrapWith<'(',')'>{};
struct SqBracks:WrapWith<'[',']'>{};
struct Bracks  :WrapWith<'{','}'>{};
struct Bars    :WrapWith<'|','|'>{};
struct Tag     :WrapWith<'<','>'>{};

// ItemDef<Bars,Parens,Bracks> testItem;

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

    cout<<Has<APIOf<ItemAPI,Tag>,Tag>::value<<endl;
    // cout<<CheckRules<std::decay<decltype(testItem)>>::value<<endl;
    // cout<<Validate<std::decay<decltype(testItem)>>::run()<<endl;
    // testItem.api("*");
    cout<<endl;
    return 0;
  }
#endif
