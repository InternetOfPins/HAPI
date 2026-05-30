/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief flat modular API, static and non-recursive API interface
 * @version 1
 * @date 2025-04-10
 * 
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

template<typename Cfg=Nil>
struct ItemAPI:Cfg{
  template<typename Out>
  static constexpr void print(Out& out) {out<<"/";}
};

template<typename... OO>
struct ItemDef:APIOf<ItemAPI<>,OO...>{
  using Base=APIOf<ItemAPI<>,OO...>;
  using Base::Base;
  static constexpr const size_t size{sizeof...(OO)};
};

//stream output for items --
template<typename Out,typename... OO>
Out& operator<<(Out& out,const ItemDef<OO...>& o) {o.print(out);return out;}

//rules ItemDef query specialization --
template<typename Q,typename... OO>
constexpr const bool query<Q,ItemDef<OO...>>{(query<Q,OO>||...)};

struct A {
  template<typename O>
  struct Part:O {
    using Base=O;
    using Base::Base;
    template<typename Out> 
    static constexpr void print(Out& out) {
      out<<"/A";
      Base::print(out);
    }
  };
};

template<typename Q,typename At> constexpr const bool requires{query<Q,At>};
template<typename Q,typename At> constexpr const bool excludes{!query<Q,At>};

struct B {
  template<typename O>
  struct Part:O {
    using Base=O;
    using Base::Base;
    template<typename Out>
    static constexpr void print(Out& out) {
      out<<"/B";
      Base::print(out);
    }
  };
  template<typename Before,typename After>
  static constexpr bool rules() {
    static_assert(query<SameAs<A>,Before>,"B only makes sense after A somehow :D");
    static_assert(!query<SameAs<B>,After>,"do not repeat B!");
    static_assert(!query<SameAs<A>,After>,"A must be before B");
    return true;
  }
};

constexpr ItemDef<A,B> ok{};
// constexpr ItemDef<B> fail_requireA{};//will fail with compile error "error: static assertion failed: B only makes sense after A somehow :D"
// constexpr ItemDef<B,A> fail_order{};//will fail with compile error "error: static assertion failed: A must be before B"
// constexpr ItemDef<A,B,B> fail_unicity{};//will fail with compile error "error: static assertion failed: do not repeat B!""

void run() {
  cout<<std::boolalpha;
  cout<<"HasRules<A>:"<<HasRules<A>::value<<endl;
  cout<<"HasRules<B>:"<<HasRules<B>::value<<endl;
  cout<<"query<SameAs<A>,A>:"<<query<SameAs<A>,A><<endl;
  cout<<"query<SameAs<A>,Chain<A>>:"<<query<SameAs<A>,Chain<A>><<endl;
  cout<<ok<<endl;
  cout<<endl;
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
 