#pragma once

#include "hapi/parts.h"

namespace hapi {
  template<typename T,bool> struct _CanPrint;
  template<typename T> 
  struct _CanPrint<T,true> {
    template<typename C> static std::true_type chk(decltype(C{}.operator<<(std::declval<const hapi::Nil>())));
    template<typename C> static std::false_type chk(...);
    using type=decltype(chk<T>(T{}));
    static const bool value=type::value;
  };

  template<typename T> 
  struct _CanPrint<T,false> {
    static const bool value=false_type{};
  };

  template<typename T> 
  using CanPrint=_CanPrint<T,std::is_class<T>::value>;

  template<typename Out,typename T>
  When<CanPrint<T>::value ,Out>& operator<<(Out& out,const T& o) {return o.operator<<(out);}

};