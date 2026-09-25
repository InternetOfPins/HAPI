// the XXXDef entries (hapi::Expand / HasOwnRules) for a Def in every kind of enclosing namespace: the translator closes the
// namespaces around it, specializes in hapi::, and reopens them with the same opener; names are qualified from the root
#include "common.h"
#include <hapi/hapi.h>
struct T {int f() const {return 0;}};
struct A {int f() const {return 1+super::f();}};
struct Nil : A:final T {};                                        // global; its name also exists as hapi::Nil
namespace a::b { struct X : A:final T {}; }                       // nested namespace definition: one '}' closes both
namespace v { inline namespace v1 { template<int k> struct Y : A:final T {}; } }   // inline: reopened as inline
namespace { struct Z : A:final T {}; }                            // anonymous: reopening it reopens the same one
namespace o { namespace { struct U : A:final T {}; } }            // anonymous inside a named one
struct Outer { struct In : A:final T {}; };                       // nested in a class: no entry (a warning), a leaf
static_assert(hapi::Validates<Nil>::value && hapi::Validates<a::b::X>::value && hapi::Validates<v::Y<1>>::value
           && hapi::Validates<Z>::value && hapi::Validates<o::U>::value, "each Def expands as its APIOf");
static_assert(std::is_same<hapi::Expand<a::b::X>::Children, hapi::Chain<T,A>>::value, "children of APIOf<T,A>");
static_assert(!hapi::Validates<Outer::In>::value, "nested in a class: a leaf, as the warning says");
int main() {
  CHECK((::Nil{}.f()==1 && a::b::X{}.f()==1 && v::Y<1>{}.f()==1 && v::v1::Y<2>{}.f()==1 && Z{}.f()==1 && o::U{}.f()==1 && Outer::In{}.f()==1));
  DONE("def_namespaces");
}
