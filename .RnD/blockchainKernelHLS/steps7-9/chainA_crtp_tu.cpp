#include "components_portable.h"

// CRTP-at-terminal, exact idiom from examples/crtp/src/main.cpp:
// struct ItemDef<OO...>:APIOf<Item<CRTP<APIOf<Item<>,OO...>>>,OO...> {};
template<typename... OO>
struct ChainDef2 : APIOf<BlockAPI<CRTP<APIOf<BlockAPI<>,OO...>>>, OO...> {};

using ChainA2 = ChainDef2<SimpleConsensus, Validation, Hash, Transaction, State>;

extern "C" bool chainA2_decide(ChainA2* c) { return c->decide(); }
extern "C" uint32_t chainA2_hash(ChainA2* c) { return c->hash(); }
extern "C" bool chainA2_validate(ChainA2* c) { return c->validate(); }
