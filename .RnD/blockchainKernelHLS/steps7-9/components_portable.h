#pragma once
#include "hapi/hapi.h"
using namespace hapi;
#include <stdint.h>

// Terminal fallback API — deliberately empty. Any Consensus/Validation call
// that isn't backed by an earlier feature fails right at that call site
// (no-inner-static-assert principle), not inside library internals.
template<typename Cfg=Nil>
struct BlockAPI : Cfg {
  using Base = Cfg;
  using Base::Base;
};

// ---- leaf feature: Transaction ----
struct Transaction {
  template<typename O>
  struct Part : O {
    using Base = O;
    using Base::Base;
    uint32_t amount{0};
    uint32_t nonce{0};
    uint32_t payload() const { return amount ^ (nonce << 16); }
  };
};

// ---- Hash: reads Transaction, which must be reachable BELOW it (After) ----
struct Hash {
  template<typename O>
  struct Part : O {
    using Base = O;
    using Base::Base;
    uint32_t hash() const {
      uint32_t h = this->payload();       // Transaction::Part member
      h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
      return h;
    }
  };
  template<typename Before, typename After>
  static constexpr bool rules() {
    static_assert(Requires<SameAs<Transaction>, After>,
      "Hash: Transaction must be reachable below Hash (After) so hash() can call payload()");
    return true;
  }
};

// ---- Validation: reads Hash + Transaction, both must be below it ----
struct Validation {
  template<typename O>
  struct Part : O {
    using Base = O;
    using Base::Base;
    bool validate() const { return this->amount > 0 && this->hash() != 0; }
  };
  template<typename Before, typename After>
  static constexpr bool rules() {
    static_assert(Requires<SameAs<Hash>, After>,
      "Validation: Hash must be reachable below Validation (After)");
    static_assert(Requires<SameAs<Transaction>, After>,
      "Validation: Transaction must be reachable below Validation (After)");
    return true;
  }
};

// ---- leaf feature: State ----
struct State {
  template<typename O>
  struct Part : O {
    using Base = O;
    using Base::Base;
    uint32_t balance{1000};
  };
};

// ---- Consensus family A: needs Validation + State below it ----
struct SimpleConsensus {
  template<typename O>
  struct Part : O {
    using Base = O;
    using Base::Base;
    // reaches four different roles purely via inherited `this->` access —
    // no glue code, no self-type, no registry
    bool decide() const { return this->validate() && this->balance >= this->amount; }
  };
  template<typename Before, typename After>
  static constexpr bool rules() {
    static_assert(Requires<SameAs<Validation>, After>, "SimpleConsensus: needs Validation");
    static_assert(Requires<SameAs<State>, After>,       "SimpleConsensus: needs State");
    return true;
  }
};

// ---- Consensus family B: a substantially different rule, same dependency shape ----
struct AlternativeConsensus {
  template<typename O>
  struct Part : O {
    using Base = O;
    using Base::Base;
    bool decide() const { return this->balance >= 2 * this->amount; } // no validate() call
  };
  template<typename Before, typename After>
  static constexpr bool rules() {
    static_assert(Requires<SameAs<State>, After>, "AlternativeConsensus: needs State");
    return true;
  }
};
