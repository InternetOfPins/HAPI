// amendment: the duplicate-layer check is an exact type match: two specializations of one template are two layers
#include "common.h"
struct Term {int get() const {return 0;}};
template<int k> struct Add {int get() const {return k+super::get();}};
struct Z : Add<1>:Add<2>:final Term {};
int main() { CHECK(Z{}.get()==3); DONE("distinct_args"); }
