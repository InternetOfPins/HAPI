#pragma once

#include "hapi/base.h"

#ifndef DEBUG
namespace hapi {
#endif
  namespace aux {
    template<typename T,bool> struct _CanPrint;
    template<typename T> 
    struct _CanPrint<T,true> {
      template<typename C> static true_type chk(decltype(C{}.operator<<(declval<const Nil>())));
      template<typename C> static false_type chk(...);
      using type=decltype(chk<T>(T{}));
      static const bool value=type::value;
    };

    template<typename T> 
    struct _CanPrint<T,false> {
      using type=false_type;
      static const bool value=type{};
    };
  };

  template<typename T> 
  using CanPrint=aux::_CanPrint<T,is_class<T>::value>;

#ifndef DEBUG
};
#endif

template<typename Out,typename T>
When<CanPrint<T>::value ,Out>& operator<<(Out& out,const T& o) {return o.operator<<(out);}

