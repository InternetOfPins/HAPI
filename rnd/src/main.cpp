#include <hapi.h>
#ifndef DEBUG
using namespace hapi;
#endif

using Idx=int;
using Sz=int;

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
};

template<typename... OO>
struct ItemDef:APIOf<ItemAPI<CRTP<ItemDef<OO...>>>>::template Parts<OO...> {
  using Base=typename APIOf<ItemAPI<CRTP<ItemDef<OO...>>>>::template Parts<OO...>;
  using Base::Base;
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
  static auto act(const O& o)->decltype(o.head().get()) {cout<<" Get:<"<<o.head()<<"> ";return o.head().get();}
};

template<typename Out,Out& out>
struct Put {
  using Res=Out;
  template<typename O>
  static Res& act(const O& o) {cout<<" Put:<"<<o.head()<<"> ";return out<<o.head();}
};

/// @brief meta agent to traverse a static tree structure
/// with given static path indexes
/// @tparam A call agent
/// @tparam i index for current level
/// @tparam path... indexes for next levels
template<typename A,Sz...> struct Walk;

template<typename A>
struct Walk<A> {
  template<typename O>
  static auto act(const O& o)->decltype(A::act(o.head())) 
    {cout<<o.head().className()<<"["<<o.head().size()<<"]=>";return A::act(o.head());}
};

template<typename A,Sz i,Sz... path>
struct Walk<A,i,path...> {
  template<typename O>
  static auto act(const O& o) ->decltype(o.template call<Walk<A,path...>,i>()) 
  {cout<<o.className()<<"["<<o.size()<<"] "<<i<<":";return o.template call<Walk<A,path...>,i>();}
  // {cout<<i<<":";return A::template act<Walk<A,path...>>(o);}
};

// type list ----------------------------------------------
template<typename...> struct StaticList {
  static constexpr Idx depth() {return 1;}
  static constexpr Sz size() {return 0;}/// size of item
  template<typename Out,char sep=' '>
  Out& operator<<(Out& out) const {return out;}
  //problem here non-exiting element return void...
  // => Lists can not be empty, because we need the API return, not void
  //or return type should be store with agent!!! can NOT! Get can retrieve from a list or diverse types!
  template<typename A,int n> static void call() {assert(false);}
  template<typename A> static void call(int) {assert(false);}
};

template<typename O,typename... OO> 
struct StaticList<O,OO...> {
  using Head=O;
  using Tail=StaticList<OO...>;
  Head m_head;
  Tail m_tail;
  const Head& head() const {return m_head;}
  const Tail& tail() const {return m_tail;}
  Head& head() {return m_head;}
  Tail& tail() {return m_tail;}
  static constexpr const char* className() {return "StaticList";}
  constexpr StaticList(){}
  constexpr StaticList(StaticList&o):m_head(o.m_head),m_tail(o.m_tail) {}
  constexpr StaticList(Head&i,OO&...ii):m_head{i},m_tail{ii...}{}
  constexpr StaticList(StaticList&&o):m_head(move(o.m_head)),m_tail(move(o.m_tail)) {}
  constexpr StaticList(Head&&i,OO&&...ii):m_head{forward<Head>(i)},m_tail{forward<OO>(ii)...}{}
  static constexpr Idx depth() {return cexMax<Idx,O::depth(),StaticList<OO...>::depth()>();}
  static constexpr Sz size() {return 1+Tail::size();}
  // Sz size(const Path path,Idx i) const
  //   {return i?m_next.size(path,i-1):m_item.size(path);}
  template<typename Out,char sep=' '>
  Out& operator<<(Out& out) const {out<<m_head;out<<sep;tail().template operator<< <Out,sep>(out);return out;}

  /// @brief static call an agent on a set of diverse return types
  /// 
  /// @tparam A the agent
  /// @tparam n target index distance
  /// @return target function result type
  template<typename A,int n>
  auto call() const ->When<!!n,decltype(tail().template call<A,n-1>())>
    {return tail().template call<A,n-1>();}
  template<typename A,int n>
  auto call() const ->When<!n,decltype(A::act(*this))>
    {return A::act(*this);}

  /// @brief runtime index step
  /// @tparam A agent type
  /// @param n 
  /// @return 
  template<typename A>
  typename A::Res call(int n) const
    {return n?tail().template call<A>(n-1):A::act(*this);}

  /// @brief call the agent
  /// @tparam A agent type
  /// @return agent given result
  template<typename A>
  auto call() const->decltype(A::act(*this))
    {return A::act(*this);}
};

template<typename Out,typename... OO>
Out& operator<<(Out& out,const StaticList<OO...>& o) {return o.operator<<(out);}

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
    auto call() const->decltype(A::act(*this))
      {return A::act(*this);}
    template<typename A,int n>
    auto call() const->decltype(m_body.template call<A,n>())
      {return m_body.template call<A,n>();}
  };
};

//------------------------------------

ItemDef<Text> i0{"ok"};

using Body=StaticList<
  ItemDef<Text>,
  ItemDef<Text>,
  ItemDef<Text>
>;

Body body{"Yes","No","Cancel"};

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


void run() {
  // cout<<i0<<endl;
  // cout<<StaticList<ItemDef<Text>>::depth()<<endl;
  // cout<<Body::size()<<endl;
  // cout<<body<<endl;
  // // body.operator<< <decltype(cout),'+'>(cout)<<endl;
  //
  // cout<<body.head()<<endl;
  // cout<<body.tail().head()<<endl;
  // cout<<body.tail().tail().head()<<endl;

  cout<<menu.body()<<endl;//print all body elements
  //use agent on static index item
  // cout<<body.template call<Get,2>()<<endl;
  // cout<<body.template call<Get,1>()<<endl;
  // cout<<body.template call<Get,0>()<<endl;

  cout<<"-------------------"<<endl;
  cout<<menu.className()<<endl;
  using Out=Put<decltype(cout),cout>;
  menu.template call<Out,3>();//
  cout<<endl;
  menu.template call<Walk<Out,3,0>>();
  cout<<endl;

  // cout<<body.template call<Get>(2)<<endl;
  // cout<<body.template call<Get>(1)<<endl;
  // cout<<body.template call<Get>(0)<<endl;
  //
  // cout<<"path /0 ="<<menu.template call<Walk<Get,0>>()<<endl;
  // cout<<"path /2/1/1 ="<<menu.template call<Walk<Get,2,1,0>>()<<endl;
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