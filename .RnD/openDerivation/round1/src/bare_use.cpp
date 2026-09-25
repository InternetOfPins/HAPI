// round 1, must NOT compile: a class that names `super` used bare (rule 7)
#include <hapi/chain.h>
struct Id    { static int f(int x) {return x;} };
struct Twice { static int f(int x) {return 2*super::f(x);} };
int main() { return Twice::f(21); }
