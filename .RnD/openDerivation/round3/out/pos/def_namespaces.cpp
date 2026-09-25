// the XXXDef entries (hapi::Expand / HasOwnRules) for a Def in every kind of enclosing namespace: the translator closes the
// namespaces around it, specializes in hapi::, and reopens them with the same opener; names are qualified from the root
#include "common.h"
#include <hapi/hapi.h>
struct T {int f() const {return 0;}};
struct A {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 1+Base::f();}};};
struct Nil : hapi::APIOf<T,A> {using Base=hapi::APIOf<T,A>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,T>>, "duplicate layer in Nil");}; using Nil_APIOf=hapi::APIOf<T,A>;  namespace hapi { template<> struct Expand< ::Nil> : Expand< ::Nil_APIOf> {}; template<> struct HasOwnRules< ::Nil> : HasOwnRules< ::Nil_APIOf> {}; }                                        // global; its name also exists as hapi::Nil
namespace a::b { struct X : hapi::APIOf<T,A> {using Base=hapi::APIOf<T,A>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,T>>, "duplicate layer in X");}; using X_APIOf=hapi::APIOf<T,A>; } namespace hapi { template<> struct Expand< ::a::b::X> : Expand< ::a::b::X_APIOf> {}; template<> struct HasOwnRules< ::a::b::X> : HasOwnRules< ::a::b::X_APIOf> {}; } namespace a::b { }                       // nested namespace definition: one '}' closes both
namespace v { inline namespace v1 { template<int k> struct Y : hapi::APIOf<T,A> {using Base=hapi::APIOf<T,A>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,T>>, "duplicate layer in Y");}; template<int k> using Y_APIOf=hapi::APIOf<T,A>; }} namespace hapi { template<auto k> struct Expand< ::v::v1::Y<k>> : Expand< ::v::v1::Y_APIOf<k>> {}; template<auto k> struct HasOwnRules< ::v::v1::Y<k>> : HasOwnRules< ::v::v1::Y_APIOf<k>> {}; } namespace v { inline namespace v1 { } }   // inline: reopened as inline
namespace { struct Z : hapi::APIOf<T,A> {using Base=hapi::APIOf<T,A>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,T>>, "duplicate layer in Z");}; using Z_APIOf=hapi::APIOf<T,A>; } namespace hapi { template<> struct Expand< ::Z> : Expand< ::Z_APIOf> {}; template<> struct HasOwnRules< ::Z> : HasOwnRules< ::Z_APIOf> {}; } namespace { }                            // anonymous: reopening it reopens the same one
namespace o { namespace { struct U : hapi::APIOf<T,A> {using Base=hapi::APIOf<T,A>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,T>>, "duplicate layer in U");}; using U_APIOf=hapi::APIOf<T,A>; }} namespace hapi { template<> struct Expand< ::o::U> : Expand< ::o::U_APIOf> {}; template<> struct HasOwnRules< ::o::U> : HasOwnRules< ::o::U_APIOf> {}; } namespace o { namespace { } }            // anonymous inside a named one
struct Outer { struct In : hapi::APIOf<T,A> {using Base=hapi::APIOf<T,A>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,T>>, "duplicate layer in In");}; };                       // nested in a class: no entry (a warning), a leaf
static_assert(hapi::Validates<Nil>::value && hapi::Validates<a::b::X>::value && hapi::Validates<v::Y<1>>::value
           && hapi::Validates<Z>::value && hapi::Validates<o::U>::value, "each Def expands as its APIOf");
static_assert(std::is_same<hapi::Expand<a::b::X>::Children, hapi::Chain<T,A>>::value, "children of APIOf<T,A>");
static_assert(!hapi::Validates<Outer::In>::value, "nested in a class: a leaf, as the warning says");
int main() {
  CHECK((::Nil{}.f()==1 && a::b::X{}.f()==1 && v::Y<1>{}.f()==1 && v::v1::Y<2>{}.f()==1 && Z{}.f()==1 && o::U{}.f()==1 && Outer::In{}.f()==1));
  DONE("def_namespaces");
}
