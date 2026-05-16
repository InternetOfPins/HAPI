/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief flat modular API, static and non-recursive API interface
 * @version 1
 * @date 2025-04-10
 * 
 */

#include <hapi.h>
#include <rules.h>
using namespace hapi;

#include <iostream>
using namespace std;

struct A {static void put(){cout<<"A";}};
struct B {static void put(){cout<<"B";}};
struct C {static void put(){cout<<"C";}};

using Test=Chain<A,B>;

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
    Test::Tail::Head::put();
    // Test::Tail::Tail::Head::put();//compile time error for out of range access
    cout<<endl;
    return 0;
  }
#endif
 