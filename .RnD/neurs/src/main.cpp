/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief flat modular API, static and non-recursive API interface
 * @version 1
 * @date 2025-04-10
 * 
 */

#include <hapi/hapi.h>
namespace {using namespace hapi;}

#include <iostream>
using namespace std;

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

//------------------------------------------------------------------------

template<typename Q, typename X>
struct XFormEngine {
  private:

  template<typename O,typename Chk=std::false_type>
  struct Impl{
    using Type=O;
    template<typename P> struct Part:P {};
  };
  
  template<typename O>
  struct Impl<O,std::true_type> {
    using Type=typename X::template XForm<O>;
    template<typename P> struct Part:P {};
  };

  public:
  template<typename O>
  using On = Impl<O,typename Q::template Check<O>>;
  
  template<typename P> struct Part : P {};
};

// Clean wrapper that defers application to target object O
template<typename Q, typename X> 
struct Transform {
  template<typename O> 
  using On = typename XFormEngine<Q, X>::template On<O>;
  template<typename O> struct Part:O {};
};

template<typename Q>
struct Query {
  template<typename O> 
  using On = typename XFormEngine<Q,typename Q::template Check<O>>::On<O>;//::Type;
};

namespace xform {
  template <template <typename> class Component>
  struct Inject {
    template <typename O> using XForm = Component<O>;
  };
}

//case -------------------------------
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

using Test=Chain<A,B>;

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

    //Query --
    cout<<"A==A:"<<SameAs<A>::Check<A>::value<<endl;//yes
    cout<<"A==B:"<<SameAs<A>::Check<B>::value<<endl;//no
    cout<<"Id<1>==Id<1>:"<<SameAs<Id<1>>::Check<Id<1>>::value<<endl;//yes
    cout<<"Id<1>==Id<2>:"<<SameAs<Id<1>>::Check<Id<2>>::value<<endl;//no
    cout<<"Test==Test:"<<SameAs<Test>::Check<Test>::value<<endl;//no
    cout<<"Test!=Test:"<<Not<SameAs<Test>>::Check<Test>::value<<endl;//no
    cout<<"int==int:"<<SameAs<int>::Check<int>::value<<endl;//no
    cout<<"A==Chain<A,B>:"<<SameAs<A>::Check<Chain<A,B>>::value<<endl;//no

    //Transform --
    Item<Transform<SameAs<A>,xform::ReplaceWith<B>>::template On<A>>::put();//replaces
    // Item<Transform<SameAs<A>,ReplaceWith<B>>::template On<C>>::put();//does not replace
    // Transform<SameAs<A>,ReplaceWith<B>>::template On<Item<A,C>>::put();
    cout<<endl;
    return 0;
  }
#endif
 