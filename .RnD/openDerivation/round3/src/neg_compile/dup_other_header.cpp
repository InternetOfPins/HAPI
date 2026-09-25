// expect: duplicate layer in Y
// Distinct: A over X, a component from another header that already holds A; X's Types are spliced
#include "dup_other.h"
struct Y : A:X:final C {};
int main() {return Y{}.f();}
