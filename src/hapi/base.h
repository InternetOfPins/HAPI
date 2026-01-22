#pragma once

#ifdef ARDUINO
  #include <Arduino.h>
#endif

#ifdef __AVR__
  #include <streamFlow.h>
  using namespace StreamFlow;
  #define cout Serial
  #define endl "\n"
#else
  #include <iostream>
  using std::cout;
  using std::endl;
#endif

#ifdef __AVR__
  #include <assert.h>
  #include "hapi/platform/avr/avr_std.h"
  using avr_std::enable_if;
  using avr_std::true_type;
  using avr_std::false_type;
  using avr_std::declval;
  using avr_std::is_class;
  using avr_std::forward;
  using avr_std::move;
  using avr_std::remove_reference;
  #else
  #include <cassert>
  #include <type_traits>
  using std::enable_if;
  using std::true_type;
  using std::false_type;
  using std::declval;
  using std::is_class;
  using std::forward;
  using std::move;
  using std::remove_reference;
  using std::operator<<;
#endif

/// @brief C++11 sugar for `std::enable_if<...>::type`
/// @tparam T result type
/// @tparam chk boolean check
template<bool chk,typename T=void> using When=typename enable_if<chk,T>::type;

/// `constexpre` abreviation
#define cex constexpr

/// @brief force constexpr max
/// @tparam T type of comparing values
/// @tparam a first value
/// @tparam b second value
/// @return the larger of the value
template<typename T,T a, T b> constexpr T cexMax() {return a>b?a:b;}

//TODO: put this defs into a user given type
using Idx=int;
using Sz=int;

template<Sz...> struct StaticPath {
  template<typename Out> Out& operator<<(Out& out) const {return out;}
};

template<Sz o,Sz... oo> struct StaticPath<o,oo...> {
  template<typename Out>
  Out& operator<<(Out& out) const 
    {return out<<"/"<<o<<StaticPath<oo...>{};}
};

template<typename Out,Sz... oo>
Out& operator<<(Out& out, const StaticPath<oo...>& p) {return p.operator<<(out);}

struct Path {
  const Idx len;
  const Sz* data;
  Sz head() const {assert(len);return data[0];}
  const Path parent() const {return Path{(Idx)(len-1),data};}
  const Path next() const {return {(Idx)(len-1),&data[1]};}
  Sz operator[](Idx i) const {return data[i];}
  operator bool() const {return len;}
};

template<typename A>
struct PathAgent:Path {
  using Path::Path;
  using Res=typename A::Res;
  template<typename O>
  typename A::Res act(O& o) const 
    {return o.template call<PathAgent>(head());}
};

template<Idx len>
struct PathData {
  Sz data[len];
  operator const Path() const {return Path{len,data};}
  Sz operator[](Idx i) const {return data[i];}
  // operator Sz() const {return len?data[0]:0;}
  operator const Sz*() const {return data;}
  const Path parent() const {return {len-1,data};}
  const Path next() const {return {len-1,&data[1]};}
  PathData<len> operator ++(int) {data[0]+=1;return *this;}
  PathData<len> operator --(int) {return data[0]-=1;return *this;}
};

template<typename Out>
Out& operator<<(Out& out,const Path& o) {
  out<<"{";
  for(Sz i=0;i<o.len;i++) {
    if(i) out<<",";
    out<<o.data[i];
  }
  return out<<"}";
}

