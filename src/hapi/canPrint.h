#pragma once

#include "hapi/parts.h"

namespace hapi {
  template<typename T,bool> struct _CanPrint;
  template<typename T> 
  struct _CanPrint<T,true> {
    template<typename C> static true_type chk(decltype(C{}.operator<<(declval<const hapi::Nil>())));
    template<typename C> static false_type chk(...);
    using type=decltype(chk<T>(T{}));
    static const bool value=type::value;
  };

  template<typename T> 
  struct _CanPrint<T,false> {
    static const bool value=false_type{};
  };

  template<typename T> 
  using CanPrint=_CanPrint<T,is_class<T>::value>;

  template<typename Out,typename T>
  When<CanPrint<T>::value ,Out>& operator<<(Out& out,const T& o) {return o.operator<<(out);}

};