// expect: duplicate layer in Y
// Distinct: A:X where X (from another header) already holds A; X's Types are spliced at instantiation
#include "dup_other.h"
struct Y : A:X {};
int main() {return Y{}.f();}
