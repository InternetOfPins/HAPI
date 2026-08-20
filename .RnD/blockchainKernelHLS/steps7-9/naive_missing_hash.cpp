#include "hapi/hapi.h"
using namespace hapi;
#include <stdint.h>

template<typename Cfg=Nil> struct BlockAPI : Cfg { using Base=Cfg; using Base::Base; };
struct Transaction { template<typename O> struct Part:O { using Base=O; using Base::Base;
  uint32_t amount{0}, nonce{0}; uint32_t payload() const {return amount ^ (nonce<<16);} }; };
struct Validation { template<typename O> struct Part:O { using Base=O; using Base::Base;
  bool validate() const {return this->amount>0 && this->hash()!=0;} }; };
struct State { template<typename O> struct Part:O { using Base=O; using Base::Base; uint32_t balance{1000}; }; };
struct SimpleConsensus { template<typename O> struct Part:O { using Base=O; using Base::Base;
  bool decide() const {return this->validate() && this->balance>=this->amount;} }; };

// missing Hash entirely -- no rules() anywhere
using ChainMissing = APIOf<BlockAPI<>, SimpleConsensus, Validation, Transaction, State>;
bool f(ChainMissing& c) { return c.decide(); }
