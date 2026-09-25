// hapi::APIOf in ':' syntax (struct-only form): the named wrapper's BASE is the fold, and the translator injects the
// constructor inheritance APIOf writes by hand. Lowered into its own namespace and checked against the real hapi::APIOf.
#include <hapi/hapi.h>
#include <type_traits>

namespace od_view {
  template<typename API, typename... OO> struct APIOf : hapi::Chain<OO...>::template Part<API> {using Base=typename hapi::Chain<OO...>::template Part<API>; using Base::Base;};
}

struct Api {static constexpr int f() {return 0;}};
struct P1 {template<typename O> struct Part:O {
 using Base=O; using Base::Base; static constexpr int f() {return 1+Base::f();}};};
struct P2 {template<typename O> struct Part:O {
 using Base=O; using Base::Base; static constexpr int f() {return 10+Base::f();}};};

using Fold = hapi::Chain<P1,P2>::Part<Api>;            // what `(OO : ... : API)` is for OO = P1, P2 (an alias cannot use ':' any more)

// same base: both wrappers derive from Chain<P1,P2>::Part<Api>, and compute the same thing
static_assert(std::is_base_of<Fold, od_view::APIOf<Api,P1,P2>>::value, "od APIOf base");
static_assert(std::is_base_of<Fold, hapi::APIOf<Api,P1,P2>>::value, "hapi APIOf base");
static_assert(std::is_same<od_view::APIOf<Api,P1,P2>::Base, hapi::APIOf<Api,P1,P2>::Base>::value, "the same Base alias");
static_assert(od_view::APIOf<Api,P1,P2>::f()==11 && hapi::APIOf<Api,P1,P2>::f()==11, "same value");
// what only the hand-written HAPI wrapper adds: Types with the API first (validation and Expand<> are not observable here)
static_assert(std::is_same<hapi::APIOf<Api,P1,P2>::Types, hapi::Chain<Api,P1,P2>>::value, "hapi APIOf Types");
static_assert(std::is_same<od_view::APIOf<Api,P1,P2>::Types, hapi::Chain<P1,P2>>::value, "the ':' APIOf's Types (inherited) has no API");
int main() {}
