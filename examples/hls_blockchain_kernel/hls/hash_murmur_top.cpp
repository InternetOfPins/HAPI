// HLS synthesis target: MurmurHash-style mixing over a Transaction, via
// Chain<MurmurHash,Transaction>::Part<Terminal> -- the Hash/Transaction
// slice of the composition already verified natively (steps 8-9 of
// HAPI/.RnD/blockchainKernelHLS/HANDOFF.md; see that file before touching
// this one). This is the first of two Hash variants -- the point of the
// pair is comparing synthesized hardware for the SAME composition
// mechanism with only Hash's mixing body swapped (hash_xorfold_top.cpp).
//
// Isolated in its own translation unit -- not part of src/, so
// PlatformIO's native build doesn't compile it, and Bambu only ever sees
// one top-level global object (same isolation rationale as hls_fir's
// fir4_top.cpp / OneParse's hls_smoke -- two same-shaped globals in one
// TU pull each other's static-init machinery into the synthesized
// footprint).

#include <hapi/hapi.h>
#include <cstdint>
#include <type_traits>
using namespace hapi;

template<typename Cfg=Nil>
struct BlockAPI : Cfg { using Base=Cfg; using Base::Base; };

struct Transaction {
  template<typename O>
  struct Part : O {
    using Base=O;
    using Base::Base;
    uint32_t amount{0};
    uint32_t nonce{0};
    uint32_t payload() const { return amount ^ (nonce << 16); }
  };
};

struct MurmurHash {
  template<typename O>
  struct Part : O {
    using Base=O;
    using Base::Base;
    uint32_t hash() const {
      uint32_t h = this->payload();
      h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
      return h;
    }
  };
};

using KernelMurmur = APIOf<BlockAPI<>, MurmurHash, Transaction>;

// Same three guards as hls_smoke: fail the build immediately, no Bambu
// run needed, if a future HAPI change breaks any of the properties the
// zero-overhead/synthesizability claim relies on.
static_assert(!std::is_polymorphic<KernelMurmur>::value,
  "no vtable can survive into synthesizable hardware");
static_assert(sizeof(KernelMurmur) == 2 * sizeof(uint32_t),
  "EBO must hold across both layers -- amount+nonce only, no hidden cost");
static_assert(std::is_trivially_destructible<KernelMurmur>::value,
  "trivial destruction required for a clean HLS top-level object");

KernelMurmur kernelMurmur;

uint32_t hashMurmurTop(uint32_t amount, uint32_t nonce) {
  kernelMurmur.amount = amount;
  kernelMurmur.nonce  = nonce;
  return kernelMurmur.hash();
}
