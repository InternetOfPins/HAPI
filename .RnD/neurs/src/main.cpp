#include <hapi/hapi.h>
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

// type transformations --
namespace xform {
  struct Identity {
    template<typename T> using Apply = T;
  };

  template<typename R> struct ReplaceWith {
    template<typename> using Apply = R;
  };
}

//-------------------------------------------
// 1. Primary template handles flat types
template<typename Q, typename O>
struct Map {
  using Check = typename Q::template Check<O>::type;
};

// 2. Single specialization handles both flat elements and nested Chains
template<typename Q, typename... OO>
struct Map<Q, Chain<OO...>> {
  using Check = Chain<typename Map<Q, OO>::Check...>;
};

template<typename Q, typename X> 
struct When {
  template<typename O>
  struct Check {
    static constexpr bool value = Q::template Check<O>::value;
    
    using type = std::conditional_t<
      value,
      typename X::template Apply<O>,
      O
    >;
    template<typename P> struct Part:P {};
  };
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
    //Map --

    cout<<"int==int:"<<SameAs<int>::Check<int>::value<<endl;

    cout<<"A==A:"<<SameAs<A>::Check<A>::value<<endl;
    cout<<"A==B:"<<SameAs<A>::Check<B>::value<<endl;
    cout<<"Id<1>==Id<1>:"<<SameAs<Id<1>>::Check<Id<1>>::value<<endl;
    cout<<"Id<1>==Id<2>:"<<SameAs<Id<1>>::Check<Id<2>>::value<<endl;

    using Test=Chain<A,B>;
    cout<<"Test==Test:"<<SameAs<Test>::Check<Test>::value<<endl;
    cout<<"Test!=Test:"<<Not<SameAs<Test>>::Check<Test>::value<<endl;

    //Transform --
    cout<<boolalpha;
    using X=When<Not<SameAs<A>>,xform::ReplaceWith<A>>;//we can store transformations
    Item<X::template Check<B>>::put();//apply to elements
    using R=Map<X,Chain<A,B,Chain<C,A>>>::Check;//use them on _Query, so they can traverse structs
    Item<R>::put();

    cout<<endl;
    return 0;
  }
#endif
 