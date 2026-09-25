#include "identity.h"
int use(X& x) {return x.a();}     // links only if both TUs mangle A:B:C the same way
