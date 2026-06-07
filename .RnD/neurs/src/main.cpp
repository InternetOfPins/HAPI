#include <hapi/hapi.h>
using hapi::Chain;
using hapi::APIOf;
using hapi::SameAs;

#ifdef __AVR__
  #include <streamFlow.h>
  #include "hapi/platform/avr/avr_std.h"
  using namespace StreamFlow;
  #define cout Serial
#else
  // #include <type_traits>
  #include <iostream>
  using namespace std;
#endif

// Small test API -----------------------------------------
struct API {
  static void nl() {cout.write('\n');}
  static void put(){nl();}
};

template<typename... OO> using Item=APIOf<API,OO...>;

struct A {
  template<typename O>
  struct Part:O {
    static constexpr void put(){cout<<"A";O::put();}
  };
};
struct B {
  template<typename O>
  struct Part:O {
    static void put(){cout<<"B";O::put();}
  };
};
struct C {
  template<typename O>
  struct Part:O {
    static void put(){cout<<"C";O::put();}
  };
};

// Variadic Logical Predicates ----------------------------
template <typename P>
struct Not {
  template <typename O>
  struct Check {
    static constexpr const bool value{!P::template Check<O>::value};
  };
};

template <typename... Ps>
struct And {
  template <typename O>
  struct Check {
    static constexpr const bool value{(Ps::template Check<O>::value && ... && true)};
  };
};

template <typename... Ps>
struct Or {
  template <typename O>
  struct Check {
    static constexpr const bool value{(Ps::template Check<O>::value || ... )};
  };
};

// Monadic Channels (Concrete Control Metaprogramming) ----
template<typename T>
struct Left {
  using Type = T;
  template<typename O> struct Check { 
    static constexpr bool value = std::is_same_v<O, Left<T>> || std::is_same_v<O, T>; 
  };
};

template<typename T>
struct Right {
  using Type = T;
  template<typename O> struct Check { 
    static constexpr bool value = std::is_same_v<O, Right<T>> || std::is_same_v<O, T>; 
  };
};

// Universal Inspection Tool ------------------------------
template<template<typename...> class Wrapper>
struct IsInstanceOf {
  template<typename O> struct Check { 
    static constexpr bool value{false}; 
  };
  template<typename... Args> struct Check<Wrapper<Args...>> { 
    static constexpr bool value{true}; 
  };
};

// Global Convenience Aliases for the Toolset
struct IsLeft  : IsInstanceOf<Left> {};
struct IsRight : IsInstanceOf<Right> {};

// Categorization Transformation ---------------------------
template<typename Q>
struct Partition {
  template<typename O>
  struct Apply {
    static constexpr bool value = Q::template Check<O>::value;
    using Expr = std::conditional_t<value, Right<O>, Left<O>>;
  };
};

// Pure 1:1 Map Topology Walker ---------------------------
template<typename F, typename O>
struct Map {
  using Expr = typename F::template Apply<O>::Expr;
};

template<typename F, typename... OO>
struct Map<F, Chain<OO...>> {
  using Expr = Chain<typename Map<F, OO>::Expr...>;
};

// Conditional Extraction and Cleanup Engine --------------
template<typename P, typename C, typename Enable = void> 
struct FilterIf;

template<typename P> 
struct FilterIf<P, Chain<>> { 
  using Expr = Chain<>; 
};

// 1. Case for Monadic Wrappers (Excludes Chains using SFINAE to prevent ambiguity)
template<typename P, template<typename...> class Wrapper, typename T, typename... Args, typename... OO>
struct FilterIf<P, Chain<Wrapper<T, Args...>, OO...>, 
                std::enable_if_t<!std::is_same_v<Wrapper<T, Args...>, Chain<T, Args...>>>> {
  static constexpr bool match = P::template Check<Wrapper<T, Args...>>::value;
  
  using Expr = std::conditional_t<
    match,
    typename FilterIf<P, Chain<OO...>>::Expr::template App<T>,
    typename FilterIf<P, Chain<OO...>>::Expr
  >;
};

// 2. Case for Recursive Sub-Chains within the hardware tree
template<typename P, typename... OO, typename... Rest>
struct FilterIf<P, Chain<Chain<OO...>, Rest...>> {
  using HeadFilter = typename FilterIf<P, Chain<OO...>>::Expr;
  using Expr = typename FilterIf<P, Chain<Rest...>>::Expr::template App<HeadFilter>;
};

void run() {
  // 1. Direct Inspection Tests using IsInstanceOf
  using SampleL = Left<A>;
  using SampleR = Right<B>;

  cout<<"Is SampleL Left?  "<<IsLeft::Check<SampleL>::value<<endl;
  cout<<"Is SampleR Right? "<<IsRight::Check<SampleR>::value<<endl;
  cout<<"Is SampleL Right? "<<IsRight::Check<SampleL>::value<<endl;

  // Complex logical composition: "Is Left AND NOT of underlying type B"
  using ComplexPredicate = And<IsLeft, Not<SameAs<B>>>;
  cout<<"Does SampleL pass the complex predicate? "<<ComplexPredicate::Check<SampleL>::value<<endl;

  // 2. Hardware Pipeline Execution
  using HardwareTopology = Chain<A, B, Chain<C, B, A>>;
  using Rule = Partition<SameAs<B>>; // Isolate pin/component B

  // 1:1 categorical mapping
  using DividedTopology = Map<Rule, HardwareTopology>::Expr;

  // Apply filters using clean global predicates
  using RightDomain = FilterIf<IsRight, DividedTopology>::Expr; // Extract matched nodes
  using LeftDomain  = FilterIf<IsLeft,  DividedTopology>::Expr; // Extract rejected nodes

  cout<<"\nRight Domain (Only B components): ";
  Item<RightDomain>::put(); // BB
  cout<<endl;

  cout<<"Left Domain (Remaining infrastructure): ";
  Item<LeftDomain>::put();  // ACA
}

#ifdef ARDUINO
  void setup() {
    Serial.begin(115200);
    while(!Serial);
  }
  void loop() {
    run();
    cout<<endl;
    delay(1000);
  }
#else
  int main() {
    cout<<boolalpha;
    run();
    cout<<endl;
    return 0;
  }
#endif
