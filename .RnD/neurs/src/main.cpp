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
    template<typename T> using XForm = T;
  };

  template<typename R> struct ReplaceWith {
    template<typename> using XForm = R;
  };
}

//-------------------------------------------
template<typename Q,typename O>
struct Query {
  using Check = typename Q::template Check<O>;
};

template<typename Q,typename... OO>
struct Query<Q,Chain<OO...>> {
  using Check = Chain<typename Q::template Check<OO>...>;
};

template<typename Q,typename X> struct Trans {
  template<typename O>
  using Check=std::conditional_t<
    Q::template Check<O>::value,
    typename X::template XForm<O>,
    O
  >;
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
    //Query --

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
    using X=Trans<Not<SameAs<A>>,xform::ReplaceWith<A>>;//we can store transformations
    Item<X::template Check<B>>::put();//apply to elements
    using R=Query<X,Chain<A,B,Chain<C,A>>>::Check;//use them on _Query, so they can traverse structs
    Item<R>::put();

    cout<<endl;
    return 0;
  }
#endif
 