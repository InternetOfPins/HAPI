// hapi::APIOf in ':' syntax: `struct APIOf : (OO : ... : final API) {}`. `final API` closes on the terminal API, and the
// real hapi::APIOf is what closes (it starts the Part collapse). Lowered into its own namespace and checked against it.
#include <hapi/hapi.h>
#include <type_traits>

namespace od_view {
  template<typename API, typename... OO> struct APIOf : (OO : ... : final API) {};
}

struct Api {static constexpr int f() {return 0;}};
struct P1 {static constexpr int f() {return 1+super::f();}};
struct P2 {static constexpr int f() {return 10+super::f();}};

using Fold = hapi::Chain<P1,P2>::Part<Api>;            // the collapse APIOf<Api,P1,P2> starts

// closed with `final API`, the ':' APIOf derives from the real one: same collapse, same value, same Types (API first)
static_assert(std::is_base_of<hapi::APIOf<Api,P1,P2>, od_view::APIOf<Api,P1,P2>>::value, "closed by hapi::APIOf");
static_assert(std::is_base_of<Fold, od_view::APIOf<Api,P1,P2>>::value && std::is_base_of<Fold, hapi::APIOf<Api,P1,P2>>::value, "same collapse");
static_assert(od_view::APIOf<Api,P1,P2>::f()==11 && hapi::APIOf<Api,P1,P2>::f()==11, "same value");
static_assert(std::is_same<od_view::APIOf<Api,P1,P2>::Types, hapi::Chain<Api,P1,P2>>::value, "same Types, API first");
int main() {}
