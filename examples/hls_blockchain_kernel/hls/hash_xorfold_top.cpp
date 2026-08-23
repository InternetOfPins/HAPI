// HLS synthesis target: shift/xor-fold mixing over the SAME Transaction,
// via Chain<XorFoldHash,Transaction>::Part<Terminal>. Only Hash's mixing
// body differs from hash_murmur_top.cpp -- Transaction, BlockAPI, and the
// composition shape are byte-for-byte identical. Deliberately has no
// multiply (shifts + xors only), unlike MurmurHash's `h *= 0x5bd1e995u` --
// predicted to route through logic fabric rather than a DSP block; see
// README.md's synthesis results for whether that prediction held.

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

struct XorFoldHash {
  template<typename O>
  struct Part : O {
    using Base=O;
    using Base::Base;
    uint32_t hash() const {
      uint32_t h = this->payload();
      h ^= (h << 7) ^ (h >> 3);
      return h;
    }
  };
};

using KernelXorFold = APIOf<BlockAPI<>, XorFoldHash, Transaction>;

static_assert(!std::is_polymorphic<KernelXorFold>::value,
  "no vtable can survive into synthesizable hardware");
static_assert(sizeof(KernelXorFold) == 2 * sizeof(uint32_t),
  "EBO must hold across both layers -- amount+nonce only, no hidden cost");
static_assert(std::is_trivially_destructible<KernelXorFold>::value,
  "trivial destruction required for a clean HLS top-level object");

KernelXorFold kernelXorFold;

uint32_t hashXorFoldTop(uint32_t amount, uint32_t nonce) {
  kernelXorFold.amount = amount;
  kernelXorFold.nonce  = nonce;
  return kernelXorFold.hash();
}
