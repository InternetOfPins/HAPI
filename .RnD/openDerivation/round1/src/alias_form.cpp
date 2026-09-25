// round 1, must be REFUSED by the translator: ':' in an alias-declaration (struct-only form, rule 4)
#include <hapi/rules.h>
struct Id    { static int f(int x) {return x;} };
struct Twice { static int f(int x) {return 2*super::f(x);} };
using My = Twice:Id;
