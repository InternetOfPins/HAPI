// single/hapi.h (scripts/amalgamate.py) on its own: CI's smoke build compiles every tests/*.cpp on each compiler it runs,
// so this checks the amalgamated header everywhere; tests/single_header/run.sh compares it against include/ binary for binary.
#include "../single/hapi.h"
using namespace hapi;

struct API {static constexpr int f() {return 0;}};
struct A {template<typename O> struct Part : O {using Base=O; using Base::Base; static constexpr int f() {return 1+Base::f();}};};
struct B {
  template<typename O> struct Part : O {using Base=O; using Base::Base; static constexpr int f() {return 10+Base::f();}};
  template<typename Before,typename After> static constexpr bool rules() {return query<SameAs<A>,Before>;}   // B only after A
};
using Z = APIOf<API,A,B>;

static_assert(Z::f()==11, "the chain collapses");
static_assert(std::is_same<Z::Types,Chain<API,A,B>>::value, "Types, API first");
static_assert(Validates<Z>::value && Requires<SameAs<B>,Z::Types>, "Expand / rules");
static_assert(Distinct<Chain<A,B,API>> && !Distinct<Chain<A,B,A>>, "Distinct");
int main() {return Z::f()==11 ? 0 : 1;}
