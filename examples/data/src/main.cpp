#include <hapi.h>
using namespace hapi;

///////////////////////////////////////////////////--
using Year=Parts<StaticInt<1967>,Nil>;
const char* label="label:";
NilPart<StaticText<label>,ReflexOf,FieldValue<Int>> year{2025};

void run() {
  cout<<Year::get()<<endl;

  cout<<year.changed()<<endl;
  cout<<year.getValue()<<endl;
  year.set(1901);
  cout<<year.changed()<<endl;
  cout<<year.getValue()<<endl;

  cout<<year<<endl;
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