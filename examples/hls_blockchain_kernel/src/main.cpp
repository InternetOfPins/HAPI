// Native sanity check, exactly like hls_smoke's main.cpp / fir4_top's
// implicit host driver: NOT what gets synthesized (the hls/*_top.cpp
// files are), just confirms both kernels still behave before spending a
// Bambu run on them. Re-declares the same two compositions rather than
// including the hls/*.cpp files directly, so this stays a pure
// regression check independent of the isolated-TU / single-global-per-
// file constraint those files exist under.

#include <hapi/hapi.h>
#include <cstdint>
#include <cstdio>
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

using KernelMurmur  = APIOf<BlockAPI<>, MurmurHash, Transaction>;
using KernelXorFold = APIOf<BlockAPI<>, XorFoldHash, Transaction>;

int main() {
  KernelMurmur m{}; m.amount = 500; m.nonce = 7;
  KernelXorFold x{}; x.amount = 500; x.nonce = 7;

  printf("hashMurmurTop(500,7)  = %u\n", m.hash());
  printf("hashXorFoldTop(500,7) = %u\n", x.hash());
  printf("sizeof(KernelMurmur)  = %zu\n", sizeof(KernelMurmur));
  printf("sizeof(KernelXorFold) = %zu\n", sizeof(KernelXorFold));
  return 0;
}
