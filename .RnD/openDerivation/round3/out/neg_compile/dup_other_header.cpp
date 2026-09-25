// expect: duplicate layer in Y
// Distinct: A:X where X (from another header) already holds A; X's Types are spliced at instantiation
#include "dup_other.h"
struct Y : hapi::Chain<A>::Part<X> {using Base=hapi::Chain<A>::Part<X>; using Base::Base; static_assert(hapi::Distinct<hapi::Chain<A,X>>, "duplicate layer in Y");};
int main() {return Y{}.f();}
