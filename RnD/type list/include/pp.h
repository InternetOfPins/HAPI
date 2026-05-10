/**
 * @file pp.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief 
 * @version 5
 * @date 2026-05-10
 * 
 * @copyright Copyright (c) 2026
 * 
*/

namespace one$ {
  template<typename O> struct PP {
    using Type=O;
    const Type& value;
    constexpr PP(Type& o):value{o}{}
  };

  template<> struct PP<const char*> {
    using Type=const char*;
    const Type value;
    constexpr PP(Type o):value{o}{}
  };

  template<typename Out> Out& operator<<(Out& out,PP<const char> o) {return out<<'\''<<o.value<<'\'';}
  template<typename Out> Out& operator<<(Out& out,PP<const char*> o) {return out<<'"'<<o.value<<'"';}
  template<typename Out,int n> Out& operator<<(Out& out,PP<const char[n]> o) {return out<<'"'<<o.value<<'"';}
  template<typename Out,typename T> Out& operator<<(Out& out,PP<const T> o) {return out<<o.value;}
  template<typename Out,typename T> Out& operator<<(Out& out,PP<T> o) {return out<<o.value;}
};//namespce one$