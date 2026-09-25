// expect: duplicate layer in Y
// Distinct: A over X, a component from another header that already holds A; X's Types are spliced
#include "dup_other.h"
struct Y : hapi::APIOf<C,A,X> {using Base=hapi::APIOf<C,A,X>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,X,C>>, "duplicate layer in Y");};
int main() {return Y{}.f();}
