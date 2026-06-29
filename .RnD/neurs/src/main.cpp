#include <hapi/hapi.h>
using namespace hapi;

#ifdef __AVR__
  #include <streamFlow.h>
  #include "hapi/platform/avr/avr_std.h"
  using namespace StreamFlow;
  #define cout Serial
#else
  // #include <type_traits>
  #include <iostream>
  using namespace std;
#endif

// Small test API -----------------------------------------
struct API {
  static void nl() {endl(cout);}
  static void put(){nl();}
};

template<typename... OO> using Item=APIOf<API,OO...>;

struct A {
  template<typename O>
  struct Part:O {
    static constexpr void put(){cout<<"A";O::put();}
  };
};
struct B {
  template<typename O>
  struct Part:O {
    static void put(){cout<<"B";O::put();}
  };
};
struct C {
  template<typename O>
  struct Part:O {
    static void put(){cout<<"C";O::put();}
  };
};

void run() {
  cout<<boolalpha;
  using R  = Any<Not<SameAs<B>>, Chain<B,Chain<B,B>,B>>;
  cout<<" Any<Not<SameAs<B>>, Chain<B,Chain<B,B>,B>>: "<<R::value<<endl;

  using F  = FindFirst<SameAs<A>, Chain<B,Chain<B,A>,B>>;
  cout<<"FindFirst<SameAs<A>, Chain<B,Chain<B,A>,B>>: "<<isJust<F><<endl;           // true
  using F2 = FindFirst<SameAs<A>, Chain<B,Chain<B,B>,B>>;
  cout<<"FindFirst<SameAs<A>, Chain<B,Chain<B,B>,B>>: "<<isJust<F2><<endl;          // false

  // Transform: Tag<I> -> Tag<I+1>
  using T  = Transform<Succ, Chain<Tag<0>,Chain<Tag<1>,Tag<2>>>>;
  // T == Chain<Tag<1>,Chain<Tag<2>,Tag<3>>>
  using T0 = Eval<SameAs<Tag<1>>, T>;
  cout<<"               Any<SameAs<Tag<3>>,T>::value: "<<Any<SameAs<Tag<3>>,T>::value<<endl;  // true
  cout<<"                         query<SameAs<A>,A>: "<<query<SameAs<A>,A><<endl;
}

#ifdef ARDUINO
  void setup() {
    Serial.begin(115200);
    while(!Serial);
  }
  void loop() {
    run();
    cout<<endl;
    delay(1000);
  }
#else
  int main() {
    cout<<boolalpha;
    run();
    cout<<endl;
    return 0;
  }
#endif
