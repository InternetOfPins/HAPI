#include "components_portable.h"
// missing Hash entirely -- Validation's rules() should catch it
using ChainMissing = APIOf<BlockAPI<>, SimpleConsensus, Validation, Transaction, State>;
bool f(ChainMissing& c) { return c.decide(); }
