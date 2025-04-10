/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief flat modular API, free use of parts, a composition of parts is also a valid part.
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

using Parens  =WrapWith<'(',')'>;
using SqBracks=WrapWith<'[',']'>;
using Bracks  =WrapWith<'{','}'>;
using Bars    =WrapWith<'|','|'>;
using Tag     =WrapWith<'<','>'>;

//a composition of parts is also a part
//until it is closed with a terminator
using All=Parts<Parens,SqBracks,Bracks,Bars,Tag>;

//using terminator `Item`
Parts<All,Item> testItem;

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
