// wave4: the trained Banknote wave cell of fold 1 (../models/banknote/net.h, wave_params.h): no multiply, no table, no RAM.
#include "net.h"
__attribute__((noinline)) bool bnc_predict(const uint8_t* x){ wave::Features<4> f{{x[0],x[1],x[2],x[3]}}; return BanknoteNet::proc(f); }
