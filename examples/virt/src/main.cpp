/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief composing items with common virtual interface
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

struct Item {
  static void api(const char*o) {cout<<o;}
};

template<char oc, char cc>
struct WrapWith {
  template<typename I>
  struct Part:I {
    static void api(const char*o) {
      cout<<oc;
      I::api(o);
      cout<<cc;
    }
  };
};

using Parens=WrapWith<'(',')'>;
using SqBracks=WrapWith<'[',']'>;
using Bracks=WrapWith<'{','}'>;
using Bars=WrapWith<'|','|'>;
using Tag=WrapWith<'<','>'>;

/**/
struct IItem {
  virtual void api(const char*) const=0;
};

template<typename... OO> struct ItemDef;

template<typename... OO>
struct IItemOf:IItem,ItemDef<OO...> {
  using Base=ItemDef<OO...>;
  void api(const char*o) const override {Base::api(o);}
};

template<typename... OO>
struct ItemDef:APIOf<Item>::template Parts<OO...> {};

ItemDef<Bars,Parens,SqBracks,Bracks,Tag> testItem;

using ItemA=IItemOf<Tag,Bars>;
using ItemB=IItemOf<Bracks,SqBracks>;

ItemA a;
ItemB b;

//generic IItem container with virtual divergent items
IItem* list[]{&a,&b};

void run() {
  testItem.api("*");cout<<endl;
  a.api("a");cout<<endl;
  b.api("b");cout<<endl;
  list[0]->api("0");cout<<endl;
  list[1]->api("1");cout<<endl;
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
