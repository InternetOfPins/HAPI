#pragma once

namespace hapi {
  // Lightweight AndAll
  template<bool... Bs> struct AndAll {static constexpr bool value = true;};

  template<bool H, bool... T> struct AndAll<H, T...> 
    {static constexpr bool value = H && AndAll<T...>::value;};

  // Logical OR - returns true if ANY is true
  template<bool... Bs> 
  struct OrAny { static constexpr bool value = false;};

  template<bool H, bool... T> 
  struct OrAny<H, T...> 
    {static constexpr bool value = H || OrAny<T...>::value;};

  //type list, compile time, can be empty--
  template<typename... Ts>
  struct TypeList {
    static constexpr size_t size = sizeof...(Ts);

    template<typename T>
    static constexpr bool Has = (std::is_same_v<T, Ts> || ...);

    //prefix/suffix or insert/append, Grok?
    template<typename... OO>
    using Append = TypeList<Ts..., OO...>;

    template<typename... OO>
    using Prepend = TypeList<OO..., Ts...>;

    template<typename O> struct Join:TypeList<Ts...,O>{};
    template<typename... OO> struct Join<TypeList<OO...>>:TypeList<Ts...,OO...>{};
  };

}//namespace hapi

// Has<> at global namespace so that it can be specialized anywhere (at global)
// template<typename Comp,typename Chk>
//   struct Has {static constexpr bool value = false;};

//this will silently fail if the type is unknown, but later be am APIOf specialization
// a bit too late
// template<typename Comp,typename T>
//   struct Same {static constexpr bool value = false;};

// too much boiler-plate?, negate locally?
// template<typename O, typename Tag> 
// struct Deny {static constexpr bool value =!Has<O, Tag>::value;};

