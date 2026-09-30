// wave4: the trained Banknote wave cell of fold 1 (../models/banknote/net.h, wave_params.h): no multiply, no table, no RAM.
#include "net.h"
__attribute__((noinline)) bool bnc_predict(const uint8_t* x){ BanknoteState f = banknote({x[BNC_COL_VARIANCE],x[BNC_COL_SKEWNESS],x[BNC_COL_CURTOSIS],x[BNC_COL_ENTROPY]}); return BanknoteNet::proc(f); }
