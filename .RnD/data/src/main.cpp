/**
 * @file main.cpp
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief DEPRECATED, will split this code somewhere else
 * @version 5
 * @date 2026-05-10
 * 
 * @copyright Copyright (c) 2026
 * 
*/

#include <hapi/hapi.h>
using namespace hapi;

///////////////////////////////////////////////////--
using Year=Chain<StaticInt<1967>,Nil>;
const char* label="label:";
NilPart<StaticText<label>,ReflexOf,FieldValue<Int>> year{2025};

int ano=1911;

//use StaticData to store a reference
NilPart<StaticData<const int&,ano>> a;

void run() {
  cout<<Year::get()<<endl;

  cout<<year.changed()<<endl;
  cout<<year.getValue()<<endl;
  year.set(1901);
  cout<<year.changed()<<endl;
  cout<<year.getValue()<<endl;

  cout<<year<<endl;

  cout<<a<<endl;
  ano=1987;
  cout<<a<<endl;

}

#ifdef ARDUINO
  void setup() {
    Serial.begin(115200);
    while(!Serial);
  }
  void loop() {
    run();
    delay(500);
  }
#else
  int main() {
    run();
    return 0;
  }
#endif