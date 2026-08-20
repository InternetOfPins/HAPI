#include "components_portable.h"

using ChainA = APIOf<BlockAPI<>, SimpleConsensus, Validation, Hash, Transaction, State>;

// force instantiation + external linkage entry points so the linker/nm
// actually has to keep these symbols around (no full inlining into a
// vanished local)
extern "C" bool chainA_decide(ChainA* c) { return c->decide(); }
extern "C" uint32_t chainA_hash(ChainA* c) { return c->hash(); }
extern "C" bool chainA_validate(ChainA* c) { return c->validate(); }
