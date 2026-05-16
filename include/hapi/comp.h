#pragma once

#include "hapi/base.h"

#pragma once

namespace hapi {

  template<template<typename...> class Interface, typename... Ts>
  struct Composite;

  template<template<typename...> class Interface, typename T,typename... Ts>
  struct Composite<Interface,T,Ts...> {
    static constexpr size_t size = sizeof...(Ts)+1;

    using Head=T;
    using Tail=Interface<Ts...>;

    template<typename O>
    static constexpr bool Has = std::is_same_v<O, T> ||(std::is_same_v<O, Ts> || ...);

    //this is a cargo transfer, overkill?
    template<template<typename...> class Template>
    using Build = Template<Ts...>;

    // Append / Prepend
    template<typename... OO > using App = Interface<T, Ts..., OO...>;
    template<typename... OO> using Ins = Interface<OO..., T, Ts...>;

    // Concat
    template<typename... Us> using Concat = Interface<T,Ts..., Us...>;

  };

  template<template<typename...> class Interface>
  struct Composite<Interface> {
    static constexpr size_t size = 0;
    template<typename T> static constexpr bool Has = false;
    template<template<typename...> class Template>
    using Build = Template<>;

    // Append / Prepend
    template<typename... OO> using App = Interface<OO...>;
    template<typename... OO> using Ins = Interface<OO...>;

    // Concat
    template<typename... Us> using Concat = Interface<Us...>;

  };

  //generic composite
  template<typename... Ts>
  struct TypeList:Composite<TypeList,Ts...>{};
  
};//namespace hapi