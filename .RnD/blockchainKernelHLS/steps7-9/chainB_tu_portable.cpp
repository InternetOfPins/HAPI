#include "components_portable.h"

using ChainB = APIOf<BlockAPI<>, AlternativeConsensus, Validation, Hash, Transaction, State>;

extern "C" bool chainB_decide(ChainB* c) { return c->decide(); }
extern "C" uint32_t chainB_hash(ChainB* c) { return c->hash(); }
extern "C" bool chainB_validate(ChainB* c) { return c->validate(); }
