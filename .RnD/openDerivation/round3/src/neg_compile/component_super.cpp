// expect: error
// the component's B calls super::missing(); closed on the user's Nil, using it fails (Nil has no missing)
#include <hapi/hapi.h>
struct Nil {};
struct B {int h() const {return 5;} int g() const {return super::missing();}};
struct A {int f() const {return 1+super::h();}};
struct W : A:B {};
struct Z : W:final Nil {};
int main() {return Z{}.g();}
