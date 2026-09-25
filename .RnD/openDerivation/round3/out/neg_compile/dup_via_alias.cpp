// expect: duplicate layer in Z
// Distinct: the same layer under two names (an alias, as static_net's Wave<i,...> = WaveOf<Slot<i>,...>)
#include <hapi/hapi.h>
struct T {int f() const {return 0;}};
template<int i> struct Slot {};
template<typename S> struct WaveOf {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int f() const {return 1+Base::f();}};};
template<int i> using Wave = WaveOf<Slot<i>>;
struct Z : hapi::APIOf<T,Wave<0>,WaveOf<Slot<0>>> {using Base=hapi::APIOf<T,Wave<0>,WaveOf<Slot<0>>>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Wave<0>,WaveOf<Slot<0>>,T>>, "duplicate layer in Z");}; using Z_APIOf=hapi::APIOf<T,Wave<0>,WaveOf<Slot<0>>>;  namespace hapi { template<> struct Expand<Z> : Expand<Z_APIOf> {}; template<> struct HasOwnRules<Z> : HasOwnRules<Z_APIOf> {}; }
int main() {return Z{}.f();}
