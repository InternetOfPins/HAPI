#include <hapi.h>
using namespace hapi;

using Idx=int;
using Sz=int;

template<typename I=Nil>
struct ItemAPI:I {
  static constexpr Idx depth() {return 1;}
  static constexpr Sz size() {return 1;}/// size of item
  template<typename Out> static bool printTo(Out& out) {return false;}
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
struct Get {
  template<typename O>
  static auto call(const O& o)->decltype(o.head().get()) {return o.head().get();}
};

template<typename A,Sz...> struct Walk;

template<typename A>
struct Walk<A> {
  template<typename O>
  static auto call(const O& o)->decltype(A::call(o)) {return A::call(o);}
};

template<typename A,Sz i,Sz... path>
struct Walk<A,i,path...> {
  template<typename O>
  static auto call(const O& o) ->decltype(o.template call<Walk<A,path...>,i>()) 
    {return o.template call<Walk<A,path...>,i>();}
};

// type list ----------------------------------------------
template<typename...> struct StaticList {
  static constexpr Idx depth() {return 1;}
  static constexpr Sz size() {return 0;}/// size of item
  template<typename Out,char sep=' '>
  Out& operator<<(Out& out) const {return out;}
  template<typename A,int n>
  static void call() {assert(false);}
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
  constexpr StaticList(){}
  constexpr StaticList(StaticList&&o):m_head(move(o.m_head)),m_tail(move(o.m_tail)) {}
  constexpr StaticList(Head&&i,OO&&...ii):m_head{forward<Head>(i)},m_tail{forward<OO>(ii)...}{}
  static constexpr Idx depth() {return cexMax<Idx,O::depth(),StaticList<OO...>::depth()>();}
  static constexpr Sz size() {return 1+Tail::size();}
  // Sz size(const Path path,Idx i) const
  //   {return i?m_next.size(path,i-1):m_item.size(path);}
  template<typename Out,char sep=' '>
  Out& operator<<(Out& out) const {out<<m_head<<sep;return m_tail.template operator<< <Out,sep>(out);}

  template<typename A,int n>
  auto call() const ->When<!!n,decltype(tail().template call<A,n-1>())>
    {return tail().template call<A,n-1>();}
  template<typename A,int n>
  auto call() const ->When<!n,decltype(A::call(*this))>
    {return A::call(*this);}

  template<typename A>
  auto call() const->decltype(A::call(*this))
    {return A::call(*this);}
};

template<typename Out,typename... OO>
Out& operator<<(Out& out,const StaticList<OO...>& o) {return o.operator<<(out);}

template<typename T,typename B>
struct Menu:T {
  // template<typename O>
  // struct Part:T::template Part<O> {
  //   using Base=typename T::template Part<O>;
  //   using Title=Base;
  using Base=T;
  using Title=Base;
  using Body=B;
    // Body body;
    Menu(T t,B b):T{t},B{b}{}
    Body m_body;
    using Base::Base;
    cex Menu(Base&&t,B&&b):Base{forward<Base>(t)},m_body{forward<B>(b)}{}
    template<typename... OO>
    cex Menu(Base&&t,OO&&... oo):Base{forward<Base>(t)},m_body{forward<OO>(oo)...}{}
    static constexpr Idx depth() {return 1+Body::depth();}
    static constexpr Sz size() {return Body::sz();}
  // };
};

//------------------------------------

ItemDef<Text> i0{"ok"};

using Body=StaticList<
  ItemDef<Text>,
  ItemDef<Text>,
  ItemDef<Text>
>;

Menu<
  ItemDef<Text>,
  ItemDef<Text>
// ItemDef<StaticList<
//     ItemDef<Text>,
//     ItemDef<Text>
//   >>
> menu{"Sub-menu","zZz"};

Body body{"Yes","No","Cancel"};

void run() {
  cout<<i0<<endl;
  cout<<StaticList<ItemDef<Text>>::depth()<<endl;
  cout<<Body::size()<<endl;
  cout<<body<<endl;
  body.operator<< <decltype(cout),'+'>(cout)<<endl;

  cout<<body.head()<<endl;
  cout<<body.tail().head()<<endl;
  cout<<body.tail().tail().head()<<endl;

  //static index
  cout<<body.template call<Get,2>()<<endl;
  cout<<body.template call<Get,1>()<<endl;
  cout<<body.template call<Get,0>()<<endl;

  // cout<<"path="<<body.template call<Walk<Get,3,1>>()<<endl;
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