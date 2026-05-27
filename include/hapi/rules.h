/**
 * @file rules.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief 
 * 
*/

#pragma once

template<typename...> struct Chain;

//query --
template<typename Q,typename O>
constexpr const bool query{Q::template Check<O>::value};

//predicates --
template<typename Q> struct SameAs {
  template<typename O> struct Check{
    static constexpr const bool value{std::is_same_v<O,Q>};
  };
};

//sugar predicates--
template<typename T,typename O,typename... OO>
constexpr const bool has{query<SameAs<T>,O,OO...>};

template<typename T>
constexpr const bool has<T,Chain<>>{false};

struct RulesAPI {
  template<typename Before,typename After=Chain<>>
  static constexpr bool rules() {return true;}

};

template<typename T,typename Ahead,typename Behind=Chain<>>
struct CheckRules {
  static constexpr bool check() {
    return T::template rules<Behind,Ahead>()&&
      CheckRules<typename Ahead::Head,typename Ahead::Tail,typename Behind::template App<T>>::check();
  }
};

template<typename T,typename Behind>
struct CheckRules<T,Chain<>,Behind> {
  static constexpr bool check()
    {return T::template rules<Behind,Chain<>>();}
};