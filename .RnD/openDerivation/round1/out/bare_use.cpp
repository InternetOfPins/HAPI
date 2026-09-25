// round 1, must NOT compile: a class that names `super` used bare (rule 7)
#include <hapi/rules.h>
struct Id    { static int f(int x) {return x;} };
struct Twice {template<typename O> struct Part:O {
 using Base=O; using Base::Base; static int f(int x) {return 2*Base::f(x);} };};
int main() { return Twice::f(21); }
