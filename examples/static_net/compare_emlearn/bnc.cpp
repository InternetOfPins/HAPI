// one Banknote program: -DBNC_MODEL='"models/x.h"' -DBNC_NAME='"x"' (run.sh). The rows header comes first (the column names), then the model header, which defines bnc_predict().
#include "bnc_rows.h"     // the column names (BNC_COL_*) come first: the model headers use them
#include BNC_MODEL
#include "bnc_common.h"
