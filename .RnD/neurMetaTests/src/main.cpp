#include <hapi/hapi.h>
// using hapi::Chain;
// using hapi::APIOf;
// using hapi::SameAs;
using namespace hapi;

#include <oneData/oneData.h>
using namespace oneData;

#include <oneItem/oneItem.h>
// using namespace oneItem;

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

// 1. Build your own classes
struct ItemAPI:oneItem::ItemAPI<> {static void put(){cout<<endl;}};
template<typename... OO> using Item=APIOf<ItemAPI,OO...>;

// 2. Inject your api functions oneItem::ItemAPI
// struct ItemBase {static void put(){cout<<endl;}};
// using ItemAPI=oneItem::ItemAPI<ItemBase>;
// template<typename... OO> using ItemDef=hapi::APIOf<ItemAPI,OO...>;

// template<typename... OO> using Item=ItemDef<OO...>;

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

template<int i> 
struct Id {
  template<typename I> 
  struct Part : I { 
    using I::I; 
    static void put(){cout<<"Id<"<<i<<">";I::put();}
  };
};

using O=Item<A,B,Id<1>,C> ;
O o;

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
    cout<<query<SameAs<Id<1>>,O><<endl;
    cout<<query<IsInstanceOf<Item>,O><<endl;
    cout<<IsInstanceOf<Item>::Check<O>::value<<endl;
    using Id1_p=SameAs<Id<1>>;
    using Partitioned = typename Map<Partition<Id1_p>, typename O::Types>::Expr;
    using Match = typename FilterIf<IsRight, Partitioned>::Expr;
    using NewItem=Match::Build<Item>;
    NewItem::put();
    // run();
    cout<<endl;
    return 0;
  }
#endif
