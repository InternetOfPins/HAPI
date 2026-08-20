#include "components_portable.h"

template<typename... OO>
struct ChainDef2 : APIOf<BlockAPI<CRTP<APIOf<BlockAPI<>,OO...>>>, OO...> {};

using ChainB2 = ChainDef2<AlternativeConsensus, Validation, Hash, Transaction, State>;

extern "C" bool chainB2_decide(ChainB2* c) { return c->decide(); }
extern "C" uint32_t chainB2_hash(ChainB2* c) { return c->hash(); }
extern "C" bool chainB2_validate(ChainB2* c) { return c->validate(); }
