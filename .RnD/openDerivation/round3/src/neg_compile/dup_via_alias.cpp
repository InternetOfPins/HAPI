// expect: duplicate layer in Z
// Distinct: the same layer under two names (an alias, as static_net's Wave<i,...> = WaveOf<Slot<i>,...>)
#include <hapi/rules.h>
struct T {int f() const {return 0;}};
template<int i> struct Slot {};
template<typename S> struct WaveOf {int f() const {return 1+super::f();}};
template<int i> using Wave = WaveOf<Slot<i>>;
struct Z : Wave<0>:WaveOf<Slot<0>>:T {};
int main() {return Z{}.f();}
