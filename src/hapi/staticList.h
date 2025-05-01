#pragma once

#include "hapi/base.h"

// static list ----------------------------------------------
template<typename...> struct StaticList;

template<typename Out,typename... OO>
Out& operator<<(Out& out,const StaticList<OO...>& o);

template<>
struct StaticList<> {
  static constexpr Idx depth() {return 1;}
  static constexpr Sz size() {return 0;}/// size of item
  static constexpr const char* className() {return "StaticList";}
};

template<typename O,typename... OO> 
struct StaticList<O,OO...>:StaticList<> {
  using Head=O;
  using Tail=StaticList<OO...>;
  Head m_head;
  Tail m_tail;
  const Head& head() const {return m_head;}
  Head& head() {return m_head;}
  const Tail& tail() const {return m_tail;}
  Tail& tail() {return m_tail;}
  constexpr StaticList(){}
  constexpr StaticList(StaticList&o):m_head(o.m_head),m_tail(o.m_tail) {}
  constexpr StaticList(Head&i,OO&...ii):m_head{i},m_tail{ii...}{}
  constexpr StaticList(StaticList&&o):m_head(o.head()),m_tail(move(o.m_tail)) {}
  constexpr StaticList(Head&&i,OO&&...ii):m_head{forward<Head>(i)},m_tail{forward<OO>(ii)...}{}
  static constexpr Idx depth() {return cexMax<Idx,O::depth(),StaticList<OO...>::depth()>();}
  static constexpr Sz size() {return 1+Tail::size();}
  template<typename Out,char sep=' '>
  Out& operator<<(Out& out) const
    {out<<"[";out<<head();out<<sep;::operator<< <Out>(out,tail());out<<"]";return out;}

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

