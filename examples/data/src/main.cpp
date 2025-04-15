#ifdef ARDUINO
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

///////////////////////////////////////////////////--
using Year=Parts<StaticInt<1967>,Nil>;
const char* label="label:";
NilPart<StaticText<label>,ReflexOf,FieldValue<Int>> year{2025};

int main() {
  cout<<Year::get()<<endl;

  cout<<year.changed()<<endl;
  cout<<year.getValue()<<endl;
  year.set(1901);
  cout<<year.changed()<<endl;
  cout<<year.getValue()<<endl;

  cout<<year<<endl;
  cout<<CanPrint<Year>::value<<endl;
  return 0;
}