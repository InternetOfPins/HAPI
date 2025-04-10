/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief static, recursive and cycle safe API calls
 * @date 2025-04-10
 * 
 */
#ifdef ARDUINO
  #include <Arduino.h>
#endif

#ifdef __AVR__
  #include <streamFlow.h>
  using namespace StreamFlow;
  #define cout Serial
  #define endl "\n"
#else
  #include <iostream>
  using namespace std;
#endif

#include <hapi.h>
using namespace hapi;

//item api
template<typename I=Nil>
struct Item:I {
  static void api(const char*o) {cout<<o;}
  static void api() {I::Obj::api("default");}
};

/// @brief optional item part, enclosure with given chars
/// @tparam oc open character
/// @tparam cc close character
template<char oc, char cc>
struct WrapWith {
  template<typename I>
  struct Part:I {
    using I::api;
    static void api(const char*o) {
      cout<<oc;
      I::api(o);
      cout<<cc;
    }
  };
};

/// @brief some variants of the above
using Parens=WrapWith<'(',')'>;
using SqBracks=WrapWith<'[',']'>;
using Bracks=WrapWith<'{','}'>;
using Bars=WrapWith<'|','|'>;
using Tag=WrapWith<'<','>'>;

/// @brief Item pure virtual interface
struct IItem {
  virtual void api() const=0;
  virtual void api(const char*) const=0;
};

template<typename... OO> struct ItemDef;

/// @brief virtual item type
/// @tparam ...OO parts
template<typename... OO>
struct IItemOf:IItem,ItemDef<OO...> {
  using Base=ItemDef<OO...>;
  void api() const override {Base::api();}
  void api(const char*o) const override {Base::api(o);}
};

//non virtual item type
template<typename... OO>
struct ItemDef:APIOf<
  Item<
    CRTP<APIOf<Item<>>::template Parts<OO...>>
  >
>::template Parts<OO...> {};

///////////////////////////////////////////////////////////////////////

/// @brief static item definition
ItemDef<Bars,Parens,SqBracks,Bracks,Tag> testItem;

/// @brief virtual item definitions
using ItemA=IItemOf<Tag,Bars>;
using ItemB=IItemOf<Bracks,SqBracks>;

ItemA a;
ItemB b;

void run() {
  testItem.api();cout<<endl;
  a.api("0");cout<<endl;
  b.api("1");cout<<endl;
  a.api();cout<<endl;
  b.api();cout<<endl;
}

#ifdef ARDUINO
  void setup() {
    Serial.begin(115200);
    while(!Serial);
  }
  void loop() {
    run();
    delay(1000);
  }
#else
  int main() {
    run();
    return 0;
  }
#endif
