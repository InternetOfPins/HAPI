// EXPECT-ERROR: no type named
#include "fx.h"
using R = FindFirst<SameAs<int>>::Check<Chain<A,B>>;
