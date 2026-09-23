// EXPECT-ERROR: HAPI: validation failed
#include "fx.h"
APIOf<API,APIOf<API2,BadRule>> x;
