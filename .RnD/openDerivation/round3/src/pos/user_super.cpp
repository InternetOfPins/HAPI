// rule 7: when ordinary lookup finds a `super` (the old `typedef Base super;` idiom) that one wins: the class is left alone
#include "common.h"
struct Base0  {static int f() {return 1;}};
struct Legacy : Base0 {typedef Base0 super; static int f() {return 10+super::f();}};
struct Legacy2 : Base0 {using super = Base0; static int f() {return 20+super::f();}};
int main() { CHECK(Legacy::f()==11 && Legacy2::f()==21); DONE("user_super"); }
