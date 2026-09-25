// hapi::APIOf in ':' syntax (brief, round 2 item 3): the named wrapper's BASE is the fold.
// Lowered into its own namespace and checked against the real hapi::APIOf: same base, and what the wrapper adds.
#include <hapi/hapi.h>
#include <type_traits>

namespace od_view {
  template<typename API, typename... OO> struct APIOf : hapi::Chain<OO...>::template Part<API> {};
}

struct Api {static constexpr int f() {return 0;}};
struct P1 {template<typename O> struct Part:O {
 using Base=O; using Base::Base;static constexpr int f() {return 1+Base::f();}};};
struct P2 {template<typename O> struct Part:O {
 using Base=O; using Base::Base;static constexpr int f() {return 10+Base::f();}};};

using Fold = hapi::Chain<P1,P2>::Part<Api>;                          // what `(OO : ... : API)` is for OO = P1, P2

// same base: both wrappers derive from Chain<P1,P2>::Part<Api>, and compute the same thing
static_assert(std::is_base_of<Fold, od_view::APIOf<Api,P1,P2>>::value, "od APIOf base");
static_assert(std::is_base_of<Fold, hapi::APIOf<Api,P1,P2>>::value, "hapi APIOf base");
static_assert(od_view::APIOf<Api,P1,P2>::f()==11 && hapi::APIOf<Api,P1,P2>::f()==11, "same value");
// but only the named HAPI wrapper carries Types (API first), Part<T>, validation, and an Expand<> specialization
static_assert(std::is_same<hapi::APIOf<Api,P1,P2>::Types, hapi::Chain<Api,P1,P2>>::value, "hapi APIOf Types");
static_assert(std::is_same<Fold::Types, hapi::Chain<P1,P2>>::value, "a bare fold's Types has no API");
int main() {}
