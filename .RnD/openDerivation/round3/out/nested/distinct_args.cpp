// amendment: the duplicate-layer check is an exact type match: two specializations of one template are two layers
#include "common.h"
struct Term {int get() const {return 0;}};
template<int k> struct Add {template<typename O> struct Part:O {
 using Base=O; using Base::Base; int get() const {return k+Base::get();}};};
struct Z : Add<1>::Part<Add<2>::Part<Term>> {using Base=Add<1>::Part<Add<2>::Part<Term>>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<Add<1>,Add<2>,Term>>, "duplicate layer in Z");};
int main() { CHECK(Z{}.get()==3); DONE("distinct_args"); }
