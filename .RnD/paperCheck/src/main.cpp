/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief testing
 */

#include <hapi/hapi.h>
using namespace hapi;

#ifdef __AVR__
  #include <streamFlow.h>
  using namespace StreamFlow;
  #define cout Serial
#else
  #include <iostream>
  using namespace std;
#endif

struct API {
  static constexpr void put(){}
};

struct A {
  template<typename O>
  struct Part:O {
    using Base=O;
    using Base::Base;
    static void put(){cout<<"/A";Base::put();}
  };
};
struct B {
  template<typename O>
  struct Part:O {
    using Base=O;
    using Base::Base;
    static void put(){cout<<"/B";Base::put();}
  };
};

using Test=Chain<A,B>::template Part<API>;
constexpr Test t1{};

void run() {t1.put();}

#ifdef ARDUINO
  void setup() {
    Serial.begin(115200);
    while(!Serial);
  }
  void loop() {
    t1.put();
    cout<<endl;
    delay(1000);
  }
#else
  int main() {
    run();
    cout<<endl;
    return 0;
  }
#endif
 