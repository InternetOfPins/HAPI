/**
 * @file main.cu
 * @brief Does `../hls_blockchain_kernel`'s actual point -- a protocol
 * SPECIFICATION composed from independently swappable blocks, not raw
 * hashing throughput -- survive a real CUDA device target the same way
 * it already survived Bambu HLS and Vitis HLS? Five real blocks, same
 * shape as `hls_blockchain_kernel/src/main.cpp`'s `Transaction`/
 * `MurmurHash`/`XorFoldHash` (unmodified logic, same verified hash
 * values), extended with `Validation`/`State`/`Consensus` to make this a
 * genuine multi-block specification, not a two-piece toy: the whole
 * point is proving the "swap ONE block, nothing else changes" claim
 * holds on this target too, not just testing a bare Chain<>.
 *
 * Every executable method __host__ __device__-annotated, matching
 * `../cuda_device_chain`'s own discipline. Same real, unmodified
 * hapi::Chain<>/hapi::APIOf<> pattern -- no HAPI source changes.
 */
#include <hapi/hapi.h>
#include <cstdint>
#include <cstdio>
using namespace hapi;

template<typename Cfg=Nil>
struct BlockAPI : Cfg { using Base=Cfg; using Base::Base; };

// ---- Block 1: Transaction (unchanged from hls_blockchain_kernel) ----
struct Transaction {
  template<typename O>
  struct Part : O {
    using Base=O;
    using Base::Base;
    uint32_t amount{0};
    uint32_t nonce{0};
    __host__ __device__ uint32_t payload() const { return amount ^ (nonce << 16); }
  };
};

// ---- Block 2: Hash (swappable -- unchanged logic from hls_blockchain_kernel) ----
struct MurmurHash {
  template<typename O>
  struct Part : O {
    using Base=O;
    using Base::Base;
    __host__ __device__ uint32_t hash() const {
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
    __host__ __device__ uint32_t hash() const {
      uint32_t h = this->payload();
      h ^= (h << 7) ^ (h >> 3);
      return h;
    }
  };
};

// ---- Block 3: Validation (new -- a rule over Transaction's own fields) ----
struct Validation {
  template<typename O>
  struct Part : O {
    using Base=O;
    using Base::Base;
    __host__ __device__ bool valid() const {
      return this->amount > 0 && this->amount < 1000000u;
    }
  };
};

// ---- Block 4: State (new -- a running ledger, applies the transaction) ----
struct State {
  template<typename O>
  struct Part : O {
    using Base=O;
    using Base::Base;
    uint32_t balance{0};
    __host__ __device__ uint32_t apply() {
      if (this->valid()) balance += this->amount;
      return balance;
    }
  };
};

// ---- Block 5: Consensus (new -- the decision, uses ALL other blocks) ----
struct Consensus {
  template<typename O>
  struct Part : O {
    using Base=O;
    using Base::Base;
    __host__ __device__ bool process() {
      if (!this->valid()) return false;
      uint32_t h = this->hash();
      this->apply();
      return (h % 4) != 0;
    }
  };
};

// Full specification, MurmurHash variant: Consensus -> State -> Validation -> Hash -> Transaction.
using SpecMurmur  = APIOf<BlockAPI<>, Consensus, State, Validation, MurmurHash,  Transaction>;
// Swap ONE block (Hash) -- nothing else in the specification changes.
using SpecXorFold = APIOf<BlockAPI<>, Consensus, State, Validation, XorFoldHash, Transaction>;

__global__ void kernel(uint32_t* out) {
  SpecMurmur m{};
  m.amount = 500; m.nonce = 7;
  bool accepted = m.process();
  out[0] = m.hash();       // expect 1614968633 -- matches hls_blockchain_kernel's own verified value
  out[1] = (uint32_t)m.valid();
  out[2] = m.balance;      // expect 500 (applied since valid)
  out[3] = (uint32_t)accepted;

  SpecXorFold x{};
  x.amount = 500; x.nonce = 7;
  bool acceptedX = x.process();
  out[4] = x.hash();       // expect 59186122 -- matches hls_blockchain_kernel's own verified value
  out[5] = x.balance;
  out[6] = (uint32_t)acceptedX;
}

int main() {
  // Host-side call path -- same composed types, same unmodified HAPI code.
  SpecMurmur hm{}; hm.amount = 500; hm.nonce = 7;
  bool hAccepted = hm.process();
  printf("host  MurmurSpec:  hash=%u valid=%d balance=%u accepted=%d\n",
         hm.hash(), hm.valid(), hm.balance, hAccepted);

  SpecXorFold hx{}; hx.amount = 500; hx.nonce = 7;
  bool hAcceptedX = hx.process();
  printf("host  XorFoldSpec: hash=%u balance=%u accepted=%d\n",
         hx.hash(), hx.balance, hAcceptedX);

  // Device-side call path.
  uint32_t* d_out;
  cudaMalloc(&d_out, 7 * sizeof(uint32_t));
  kernel<<<1,1>>>(d_out);
  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
    printf("kernel launch error: %s\n", cudaGetErrorString(err));
    return 1;
  }
  uint32_t h_out[7];
  cudaMemcpy(h_out, d_out, 7 * sizeof(uint32_t), cudaMemcpyDeviceToHost);
  printf("device MurmurSpec:  hash=%u valid=%u balance=%u accepted=%u\n",
         h_out[0], h_out[1], h_out[2], h_out[3]);
  printf("device XorFoldSpec: hash=%u balance=%u accepted=%u\n",
         h_out[4], h_out[5], h_out[6]);
  return 0;
}
