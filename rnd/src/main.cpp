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

template<typename...> struct StaticBody {
  static constexpr Idx depth() {return 1;}
  static constexpr Sz size() {return 0;}/// size of item
  template<typename Out,char sep=' '>
  Out& operator<<(Out& out) const {return out;}
};

template<typename O,typename... OO> 
struct StaticBody<O,OO...> {
  using Head=O;
  using Tail=StaticBody<OO...>;
  Head m_head;
  Tail m_tail;
  const Head& head() const {return m_head;}
  const Tail& tail() const {return m_tail;}
  constexpr StaticBody(){}
  constexpr StaticBody(StaticBody&&o):m_head(move(o.m_head)),m_tail(move(o.m_tail)) {}
  constexpr StaticBody(Head&&i,OO&&...ii):m_head{forward<Head>(i)},m_tail{forward<OO>(ii)...}{}
  static constexpr Idx depth() {return cexMax<Idx,O::depth(),StaticBody<OO...>::depth()>();}
  static constexpr Sz size() {return 1+Tail::size();}
  // Sz size(const Path path,Idx i) const
  //   {return i?m_next.size(path,i-1):m_item.size(path);}
  template<typename Out,char sep=' '>
  Out& operator<<(Out& out) const {out<<m_head<<sep;return m_tail.template operator<< <Out,sep>(out);}
};

template<typename Out,typename... OO>
Out& operator<<(Out& out,const StaticBody<OO...>& o) {return o.operator<<(out);}

//------------------------------------

ItemDef<Text> i0{"ok"};

using Body=StaticBody<
  ItemDef<Text>,
  ItemDef<Text>,
  ItemDef<Text>
>;

Body body{"yes","No","Cancel"};

int ano=1967;

ItemDef<StaticData<int&,ano>> a;

void run() {
  cout<<i0<<endl;
  cout<<StaticBody<ItemDef<Text>>::depth()<<endl;
  cout<<Body::size()<<endl;
  cout<<body<<endl;
  body.operator<< <decltype(cout),'+'>(cout)<<endl;

  cout<<body.head()<<endl;
  cout<<body.tail().head()<<endl;
  cout<<body.tail().tail().head()<<endl;

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