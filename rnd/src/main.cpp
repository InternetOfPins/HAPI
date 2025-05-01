#include <hapi.h>
#ifndef DEBUG
using namespace hapi;
#endif

template<typename I=Nil>
struct ItemAPI:I {
  static constexpr Idx depth() {return 1;}
  static constexpr Sz size() {return 1;}/// size of item
  template<typename Out> static bool printTo(Out& out) {return false;}
  static constexpr const char* className() {return "ItemAPI";}
  //Id--
  static constexpr int getId() {return -1;}
  template<int> using HasId=false_type;
  template<int> using WithId=ItemAPI<CRTP<ItemAPI<Nil>>>;
  template<typename Out> Out& operator<<(Out& out) {return out;}
};

template<typename... OO>
struct ItemDef:APIOf<ItemAPI<CRTP<ItemDef<OO...>>>>::template Parts<OO...> {
  using Base=typename APIOf<ItemAPI<CRTP<ItemDef<OO...>>>>::template Parts<OO...>;
  using Base::Base;
  using Base::operator<<;
};

// agents ---------------------------------------------
// @brief user agents, API can be extended and we can still use static structure traversal
// agents should be made for all future API methods that you want to use for traversal.
// some agents can operate with runtime index, they signal that with a `using Res=` => Res is uniform for all targets
// lack of it is a compile error if using runtime index.
// 
// note: Agent will receive the list tail, where target is the head()
struct Get {
  template<typename O>
  static auto act(const O& o)->decltype(o.head().get()) {return o.head().get();}
};

template<typename Out,Out& out>
struct Put {
  using Res=Out&;
  template<typename O>
  static Res& act(const O& o) {return out<<o.head();}
};

//wrap a StaticList as an Item so that we can build trees
template<typename T,typename B>
struct Menu {
  template<typename O>
  struct Part:T::template Part<O> {
    using Base=typename T::template Part<O>;
    using Title=Base;
    using Body=B;
    Body m_body;
    static constexpr const char* className() {return "Menu";}
    using Base::Base;
    // using Base::operator<<;
    Body& body() {return m_body;}
    const Body& body() const {return m_body;}
    auto head() const ->decltype(body().head()) {return body().head();}
    auto head()->decltype(body().head()) {return body().head();}
    auto tail() const ->decltype(body().tail()) {return body().tail();}
    auto tail()->decltype(body().tail()) {return body().tail();}
    // constexpr Part(Base t,Body b):Base{t},m_body{b}{}
    constexpr Part(Base&& t,Body&& b):Base{forward<Base>(t)},m_body{forward<Body>(b)}{}
    template<typename... OO>
    constexpr Part(Base&&t,OO&&... oo):Base{forward<Base>(t)},m_body{forward<OO>(oo)...}{}
    static constexpr Idx depth() {return 1+Body::depth();}
    static constexpr Sz size() {return Body::size();}

    template<typename A>
    auto call() ->decltype(A::act(body())) {return A::act(body());}

    template<typename A,Sz n>
    auto call() ->decltype(m_body.template call<A,n>())
      {return m_body.template call<A,n>();}
          
    template<typename A>
    typename A::Res call(Sz n)
      {return m_body.template call<A>(n);}
            
      template<typename Out>
    Out& operator<<(Out& out) const {
      Base::operator<<(out);
      // body().operator<<(out);
      return out;
    }
  };
};

//------------------------------------

using Body=StaticList<
  ItemDef<Text>,
  ItemDef<Text>,
  ItemDef<Text>,
  StaticList<
    ItemDef<Text>,
    ItemDef<Text>
  >
>;

Body body{"Yes","No","Cancel",{"Item A","Item B"}};

ItemDef<Menu<
  Text,//main menu
  StaticList<
    ItemDef<Text>,//yes
    ItemDef<Text>,//no
    ItemDef<Text>,//cancel
    ItemDef<Menu<
      Text,//sub-menu
      StaticList<
        ItemDef<Text>,//zzz
        ItemDef<Text>//.
      >
    >>
  >
>> menu {
  "Main menu",{
    "Yes",
    "No",
    "Cancel",{
      "Sub-menu",{
        "zZz",
        "."
      }
    }
  }
};

template<typename Out,typename...OO>
Out& operator<<(Out& out,const ItemDef<OO...>& o)
  {return o.operator<<(out);}

  //output agent for `cout`
using Out=Put<decltype(cout),cout>;

void run() {
  // body.template call<Walk<Out,3,1>>();
  // cout<<endl;
  // menu.template call<Walk<Out,3>>();
  // cout<<endl;
  // menu.template call<Walk<Out,3,0>>();
  // cout<<endl;
  // menu.template call<Walk<Out,3,1>>();
  // cout<<endl;
  // cout<<"-------------------"<<endl;
  // cout<<menu<<endl;
  // menu.template call<Out,3>();
  // cout<<endl;
  // menu.template call<Walk<Out,3,0>>();
  // cout<<endl;
  // menu.template call<Walk<Out,3,1>>();
  menu.template call<Out>(0);
  cout<<endl;
  menu.template call<Out>(1);
  cout<<endl;
  menu.template call<Out>(2);
  cout<<endl;
  menu.template call<Out>(3);
  cout<<endl;
  // menu.template call<Walk<Out>>(3,1);
  cout<<endl;
  const PathData<2> path{3,1};
  cout<<path[1]<<endl;
  const Path x(PathData<2>{3,0});
  cout<<x[0]<<endl;
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