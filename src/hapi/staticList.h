#pragma once

#include "hapi/base.h"

// static list ----------------------------------------------
template<typename,typename...> struct StaticList;

template<typename Out,typename... OO>
Out& operator<<(Out& out,const StaticList<OO...>& o);

template<typename O>
struct StaticList<O> {
  static constexpr Idx depth() {return 1;}
  static constexpr Sz size() {return 0;}/// size of item
  using Head=O;
  Head m_head;
  static constexpr const char* className() {return "StaticList";}
  const Head& head() const {return m_head;}
  Head& head() {return m_head;}
  // template<typename Out,char sep=' '>
  // Out& operator<<(Out& out) const {return out;}
  template<typename Out,char sep=' '>
  Out& operator<<(Out& out) const
    {return out<<head()<<sep;}
  constexpr StaticList(){}
  constexpr StaticList(StaticList&o):m_head(o.head()) {}
  constexpr StaticList(Head&i):m_head{i}{}
  constexpr StaticList(StaticList&&o):m_head(move(o.m_head)) {}
  constexpr StaticList(Head&&i):m_head{forward<Head>(i)}{}
  //problem here non-exiting element return void...
  // => Lists can not be empty, because we need the API return, not void
  //or return type should be store with agent!!! can NOT! Get can retrieve from a list or diverse types!
  // template<typename A,int n> static void call() {assert(false);}
  // template<typename A> static void call(int i) {assert(i==0);}
  template<typename A,Sz n>
  auto call() ->decltype(A::act(*this))
    {return A::act(*this);}
};

template<typename O,typename... OO> 
struct StaticList:StaticList<O> {
  using Base=StaticList<O>;
  using Head=O;
  using Base::head;
  using Tail=StaticList<OO...>;
  // Head m_head;
  Tail m_tail;
  // const Head& head() const {return m_head;}
  // Head& head() {return m_head;}
  const Tail& tail() const {return m_tail;}
  Tail& tail() {return m_tail;}
  // static constexpr const char* className() {return "StaticList";}
  // constexpr StaticList(){}
  // constexpr StaticList(StaticList&o):m_head(o.m_head),m_tail(o.m_tail) {}
  using Base::Base;
  constexpr StaticList(Head&i,OO&...ii):Base{i},m_tail{ii...}{}
  constexpr StaticList(StaticList&&o):Base(o.head()),m_tail(move(o.m_tail)) {}
  constexpr StaticList(Head&&i,OO&&...ii):Base{forward<Head>(i)},m_tail{forward<OO>(ii)...}{}
  static constexpr Idx depth() {return cexMax<Idx,O::depth(),StaticList<OO...>::depth()>();}
  static constexpr Sz size() {return 1+Tail::size();}
  // Sz size(const Path path,Idx i) const
  //   {return i?m_next.size(path,i-1):m_item.size(path);}
  template<typename Out,char sep=' '>
  Out& operator<<(Out& out) const
    {out<<"[";out<<head();out<<sep;::operator<< <Out>(out,tail());out<<"]";return out;}

  using Base::call;
  
  /// @brief static call an agent on a set of diverse return types
  /// 
  /// @tparam A the agent
  /// @tparam n target index distance
  /// @return target function result type
  template<typename A,Sz n>
  auto call() ->When<!!n,decltype(tail().template call<A,n-1>())>
    {return tail().template call<A,n-1>();}
  template<typename A,Sz n>
  auto call() ->When<!n,decltype(A::act(*this))>
    {return A::act(*this);}

  /// @brief runtime index step
  /// @tparam A agent type
  /// @param n 
  /// @return 
  template<typename A>
  typename A::Res call(Sz n)
    {return n?tail().template call<A>(n-1):A::act(*this);}

  /// @brief call the agent
  /// @tparam A agent type
  /// @return agent given result
  template<typename A>
  auto call() ->decltype(A::act(*this))
    {return A::act(*this);}
};

template<typename Out,typename... OO>
Out& operator<<(Out& out,const StaticList<OO...>& o)
  {return o.operator<<(out);}

/// @brief meta agent to traverse a static tree structure
/// with given static path indexes
/// @tparam A call agent
/// @tparam i index for current level
/// @tparam path... indexes for next levels
template<typename A,Sz,Sz... oo> struct Walk;
template<typename A,Sz...> struct _Walk;

template<typename A>
struct _Walk<A> {
  template<typename O>
  static auto act(O& o)->decltype(A::act(o)) 
    {return A::act(o);}
};

template<typename A,Sz i,Sz... path>
struct _Walk<A,i,path...> {
  template<typename O>
  static auto act(O& o) ->decltype(o.head().template call<_Walk<A,path...>,i>()) 
    {return o.head().template call<_Walk<A,path...>,i>();}
};

template<typename A,Sz i,Sz... oo> struct Walk:_Walk<A,i,oo...> {
  template<typename O>
  static auto act(O& o) ->decltype(o.template call<_Walk<A,oo...>,i>()) 
    {return o.template call<_Walk<A,oo...>,i>();}
};

