/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief flat modular API, static and non-recursive API interface
 * @version 1
 * @date 2025-04-10
 * 
 */

#include <hapi/hapi.h>
using namespace hapi;

#include <iostream>
using namespace std;

struct A {static void put(){cout<<"A";}};
struct B {static void put(){cout<<"B";}};
struct C {static void put(){cout<<"C";}};

using Test=Chain<A,B>;

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
    static constexpr const bool value{(Ps::template Check<O>::value || ... || false)};
  };
};

// type transformations --
namespace hapi::xform {
  struct Identity {
    template<typename T> using XForm = T;
  };

  template<typename R> struct ReplaceWith {
    template<typename> using XForm = R;
  };
}

using namespace hapi::xform;

// Core conditional transformer engine
template<typename Q, typename X, typename O, bool = Q::template Check<O>::value> 
struct XFormEngine {
  using Type = O;
};

template<typename Q, typename X, typename O>
struct XFormEngine<Q, X, O, true> {
  using Type = typename X::template XForm<O>;
};

// Clean wrapper that defers application to target object O
template<typename Q, typename X> 
struct Transform {
  template<typename O> 
  using Type = typename XFormEngine<Q, X, O>::Type;
};

namespace hapi::xform {
  template <template <typename> class Component>
  struct Inject {
    template <typename O> using XForm = Component<O>;
  };
}

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
    // Identity<A>::template XForm<>::put();
    // Replace<A>::template XForm<B>::put();
    Transform<SameAs<A>,ReplaceWith<B>>::template Type<A>::put();
    // Test::Tail::Head::put();
    // Test::Tail::Tail::Head::put();//compile time error for out of range access
    cout<<endl;
    return 0;
  }
#endif
 