// expect: error
// amendment: with an open rightmost operand the chain ends in od::Nil; using B's super-call reaches it and fails
#include <hapi/rules.h>
#include "od_nil.h"
struct B {int h() const {return 5;} int g() const {return super::missing();}};
struct A {int f() const {return 1+super::h();}};
struct Z : A:B {};
int main() {return Z{}.g();}
