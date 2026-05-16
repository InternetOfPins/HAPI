/**
 * @file typeList.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief generic list of types
 * @version 5
 * @date 2026-05-09
 * 
 * @copyright Copyright (c) 2026
 * 
*/

#ifdef ARDUINO
  #include <Arduino.h>
#endif

#ifdef __AVR__
  #include "streamFlow.h"
  using namespace StreamFlow;
  #define endl "\n"
  #define cout Serial
#else
  #include <iostream>
  using namespace std;
#endif

#include <pp.h>
using namespace one$;

template<typename O, typename... OO>
struct List {
  using Head=O;
  using Tail=List<OO...>;
  constexpr List(O o,OO... oo):m_head{o},m_tail{oo...}{}
  constexpr List(O o,Tail& t):m_head{o},m_tail{t}{}
  Head m_head;
  Tail m_tail;
  Head head() {return m_head;}
  Tail tail() {return m_tail;}
  const Head& head() const {return m_head;}
  const Tail& tail() const {return m_tail;}

  template<typename N> List<N,O,OO...> cons(N n) {return {n,*reinterpret_cast<List<O,OO...>*>(this)};}

  template<typename F,typename I>
  I foldr(const F& f,const I i) {return f(m_head,m_tail.foldr(f,i));}

  template<typename F,typename I>
  I foldl(const F& f,const I i) {return m_tail.foldl(f,f(i,m_head));}

  template<typename F>
  void map(F f,int i=0) {f(head(),i++);tail().map(f,i);}
  template<typename F>
  auto map(F f,int i=0)
    -> std::enable_if_t<
      !std::is_same_v<decltype(f(m_head,0)),void>,
      List<decltype(f(head(),i++)),decltype(tail().map(f,i))>
    > {return {f(head(),i++),tail().map(f,i)};}
  template<typename F>
  void map(F f,int i=0) const {f(head(),i++);tail().map(f,i);}
  template<typename F>
  auto map(F f,int i=0) const
    -> std::enable_if_t<
      !std::is_same_v<decltype(f(m_head,0)),void>,
      List<decltype(f(head(),i++)),decltype(tail().map(f,i))>
    > {return {f(head(),i++),tail().map(f,i)};}
};

template<typename O>
struct List<O> {
  List(O o):m_head{o}{}
  using Head=O;
  Head m_head;
  Head head() {return m_head;}
  Head head() const{return m_head;}

  template<typename N> List<N,O> cons(N n) {return {n,*this};}

  template<typename F,typename I>
  I foldr(const F& f,const I i) {return f(m_head,i);}

  template<typename F,typename I>
  I foldl(const F& f,const I i) {return f(i,m_head);}

  template<typename F> void map(F f,int i=0) {f(head(),i++);}
  template<typename F>
  auto map(F f,int i=0)
    -> std::enable_if_t<
      !std::is_same_v<decltype(f(m_head,i)),void>,
      List<decltype(f(head(),i++))>
    > {return {f(head(),i++)};}

  template<typename F>
  void map(F f,int i=0) const {f(head(),i++);}
  template<typename F>
  auto map(F f,int i=0) const
    -> std::enable_if_t<
      !std::is_same_v<decltype(f(m_head,i)),void>,
      List<decltype(f(head(),i++))>
    > {return {f(head(),i++)};}
};

// lists that can be empty... maybe later
// template<typename...>struct EList;//a list that can be empty
// template<> struct EList<> {};
// template<typename O,typename... OO>
// struct EList<O,OO...> {
//   using Head=O;
//   using Tail=EList<OO...>;
//   Head m_head;
//   Tail m_tail;
//   Head head() {return m_head;}
//   Tail tail() {return m_tail;}
// };

template<typename... OO>
constexpr List<OO...> list(OO&&... oo) {return {std::forward<OO>(oo)...};}

template<typename Out,typename O>
Out& operator<<(Out& out,const List<O>& oo)
  {return out<<'('<<PP{oo.head()}<<":[])";}

template<typename Out,typename O,typename... OO>
Out& operator<<(Out& out,const List<O,OO...>& oo)
  {return out<<'('<<PP{oo.head()}<<':'<<oo.tail()<<')';}

int ano=1967;
auto tmp{list("year",ano)};

#ifdef ARDUINO
  void setup() {
    Serial.begin(115200);
    while(!Serial);
  }
  void loop() {
    cout<<endl;
    delay(1000);
  }
#else
  int main() {
    cout<<tmp.cons("result")<<endl;
    // cout<<tmp.foldr([](auto a,auto b){},list("result"))<<endl;
    cout<<list(1,2,3).foldl([](int a,int b){return a-b;},100)<<endl;
    cout<<list(1,2,3).foldr([](int a,int b){return a-b;},100)<<endl;
    cout<<endl;
    return 0;
  }
#endif
