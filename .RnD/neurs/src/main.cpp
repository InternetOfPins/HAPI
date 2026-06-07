#include <hapi/hapi.h>
#include <type_traits>
using hapi::Chain;
using hapi::APIOf;
using hapi::SameAs;

#include <iostream>
using namespace std;

//small api -------------------------------
struct API {
  static void nl() {cout.put('\n');}
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

template<int i> 
struct Id {
  template<typename O>
  struct Part:O {
    static void put(){cout<<"C";O::put();}
  };
};

//predicate expressions--
template <typename P>
struct Not {
  template <typename O>
  struct Check {
    static constexpr const bool value{!P::template Check<O>::value};
  };
};

// Variadic AND: folds all predicates with &&
template <typename... Ps>
struct And {
  template <typename O>
  struct Check {
    static constexpr const bool value{(Ps::template Check<O>::value && ... && true)};
  };
};

// Variadic OR: folds all predicates with ||
template <typename... Ps>
struct Or {
  template <typename O>
  struct Check {
    static constexpr const bool value{(Ps::template Check<O>::value || ... )};
  };
};

// The Either Monad Channels -----------------------------
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

// Core Architectural Transformations --------------------
template<typename Q>
struct Partition {
  template<typename O>
  struct Apply {
    static constexpr bool value = Q::template Check<O>::value;
    using Expr = std::conditional_t<value, Right<O>, Left<O>>;
  };
};

// Structural Metaprogramming Core -----------------------

// 1. Pure 1:1 Map Topology Walker
template<typename F, typename O>
struct Map {
  using Expr = typename F::template Apply<O>::Expr;
};

template<typename F, typename... OO>
struct Map<F, Chain<OO...>> {
  using Expr = Chain<typename Map<F, OO>::Expr...>;
};

// 2. Right-Channel Extraction Pass
template<typename C> struct FilterRight;

template<> struct FilterRight<Chain<>> { 
  using Expr = Chain<>; 
};

template<typename T, typename... OO>
struct FilterRight<Chain<Right<T>, OO...>> {
  using Expr = typename FilterRight<Chain<OO...>>::Expr::template App<T>;
};

template<typename T, typename... OO>
struct FilterRight<Chain<Left<T>, OO...>> {
  using Expr = typename FilterRight<Chain<OO...>>::Expr;
};

template<typename... OO, typename... Rest>
struct FilterRight<Chain<Chain<OO...>, Rest...>> {
  using HeadFilter = typename FilterRight<Chain<OO...>>::Expr;
  using Expr = typename FilterRight<Chain<Rest...>>::Expr::template App<HeadFilter>;
};

// 3. Left-Channel Extraction Pass
template<typename C> struct FilterLeft;

template<> struct FilterLeft<Chain<>> { 
  using Expr = Chain<>; 
};

template<typename T, typename... OO>
struct FilterLeft<Chain<Left<T>, OO...>> {
  using Expr = typename FilterLeft<Chain<OO...>>::Expr::template App<T>;
};

template<typename T, typename... OO>
struct FilterLeft<Chain<Right<T>, OO...>> {
  using Expr = typename FilterLeft<Chain<OO...>>::Expr;
};

template<typename... OO, typename... Rest>
struct FilterLeft<Chain<Chain<OO...>, Rest...>> {
  using HeadFilter = typename FilterLeft<Chain<OO...>>::Expr;
  using Expr = typename FilterLeft<Chain<Rest...>>::Expr::template App<HeadFilter>;
};


#ifdef ARDUINO
  void setup() {
    Serial.begin(115200);
    while(!Serial);
  }
  void loop() {
    testItem.put();
    cout<<endl;
    delay(1000);
  }
#else
  int main() {
    cout<<boolalpha;

    // Direct Evaluation Checks (Outside a container)
    using SampleA = Partition<SameAs<B>>::Apply<A>::Expr; // Evaluates to Left<A>
    using SampleB = Partition<SameAs<B>>::Apply<B>::Expr; // Evaluates to Right<B>

    cout<<"Is SampleA Left?  "<<Left<A>::Check<SampleA>::value<<endl;
    cout<<"Is SampleB Right? "<<Right<B>::Check<SampleB>::value<<endl;

    // Split rule: Categorize node B vs everything else
    using Rule = Partition<SameAs<B>>;
    using HardwareTopology = Chain<A, B, Chain<C, B, A>>;

    // 1. Map walks 1:1 and partitions types into separate channels
    // Yields: Chain<Left<A>, Right<B>, Chain<Left<C>, Right<B>, Left<A>>>
    using DividedTopology = Map<Rule, HardwareTopology>::Expr;

    // 2. Extract Right Channel (Only nodes matching predicate)
    using RightDomain = FilterRight<DividedTopology>::Expr; // Chain<B, Chain<B>>
    
    // 3. Extract Left Channel (Nodes failing predicate)
    using LeftDomain  = FilterLeft<DividedTopology>::Expr;  // Chain<A, Chain<C, A>>

    cout<<"Right Domain output: ";
    Item<RightDomain>::put(); // BB
    cout<<endl;

    cout<<"Left Domain output:  ";
    Item<LeftDomain>::put();  // ACA
    cout<<endl;

    return 0;
  }
#endif

