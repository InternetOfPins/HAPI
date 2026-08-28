# cuda_blockchain_spec

`../hls_blockchain_kernel`'s actual point was never hashing throughput —
it's whether a protocol **specification** (independently swappable
blocks: `Transaction`, `Hash`, `Validation`, `State`, `Consensus`)
composes correctly and stays zero-cost, with swapping *one* block
(`Hash`, from `MurmurHash` to `XorFoldHash`) touching nothing else. That
claim already held on AVR/embedded and on Bambu+Vitis HLS. This example
asks the same question of a real CUDA device target — not "does HAPI's
bare `Chain<>` compile under `nvcc`" (`../cuda_device_chain` already
answered that with a two-piece toy), but "does a genuine multi-block
*specification* survive it."

**Answer: yes**, and more thoroughly than `cuda_device_chain`'s own
result — see PTX evidence below.

## What it does

Five real blocks, same shape and unmodified logic as
`../hls_blockchain_kernel/src/main.cpp`'s `Transaction`/`MurmurHash`/
`XorFoldHash`, extended with three new ones to make this a genuine
multi-block spec instead of a two-piece toy:

- `Transaction` — `amount`/`nonce` fields, `payload()`.
- `Hash` (**swappable**) — `MurmurHash` or `XorFoldHash`, `hash()`.
- `Validation` — a rule over `Transaction`'s own fields, `valid()`.
- `State` — a running ledger, `apply()` adds `amount` to `balance` if valid.
- `Consensus` — the decision: `process()` checks `valid()`, computes
  `hash()`, calls `apply()`, and accepts/rejects based on the hash —
  genuinely uses all four other blocks in one real call, not just
  declaring them.

`SpecMurmur` and `SpecXorFold` are the same five-block composition with
only the `Hash` block swapped — everything else, including `Consensus`'s
own logic, is untouched source.

## Real result: both call paths, both hash variants, identical

```sh
nvcc -std=c++17 -arch=sm_61 -I../../include src/main.cu -o cuda_blockchain_spec
./cuda_blockchain_spec
```

```
host  MurmurSpec:  hash=1614968633 valid=1 balance=500 accepted=1
host  XorFoldSpec: hash=59186122 balance=500 accepted=1
device MurmurSpec:  hash=1614968633 valid=1 balance=500 accepted=1
device XorFoldSpec: hash=59186122 balance=500 accepted=1
```

Host and device paths agree exactly, for both hash variants. Both hash
values match `hls_blockchain_kernel`'s own already-independently-verified
`hashMurmurTop`/`hashXorFoldTop` values exactly — confirms adding three
more blocks around `Hash` didn't perturb it, on either target.

`-arch=sm_61` targets this project's real, currently-available hardware
(GeForce GTX 1070, Pascal) — see `../cuda_device_chain/README.md` for the
history of the machine's original GPU (GT 710, Kepler) being below
CUDA's real support floor, and the driver-upgrade path that resolved it.

## PTX evidence: stronger than the bare `Chain<>` toy

```sh
nvcc -std=c++17 -arch=sm_61 -I../../include --ptx src/main.cu -o main.ptx
```

The entire kernel — both spec instances, every block's real logic
including `MurmurHash`'s actual multiply/shift mixing — is seven constant
stores:

```ptx
.visible .entry _Z6kernelPj(
	.param .u64 _Z6kernelPj_param_0
)
{
	.reg .b32 	%r<5>;
	.reg .b64 	%rd<3>;

	ld.param.u64 	%rd1, [_Z6kernelPj_param_0];
	cvta.to.global.u64 	%rd2, %rd1;
	mov.u32 	%r1, 1614968633;
	st.global.u32 	[%rd2], %r1;
	mov.u32 	%r2, 1;
	st.global.u32 	[%rd2+4], %r2;
	mov.u32 	%r3, 500;
	st.global.u32 	[%rd2+8], %r3;
	st.global.u32 	[%rd2+12], %r2;
	mov.u32 	%r4, 59186122;
	st.global.u32 	[%rd2+16], %r4;
	st.global.u32 	[%rd2+20], %r3;
	st.global.u32 	[%rd2+24], %r2;
	ret;
}
```

Zero `call`/`.func` instructions anywhere (grepped, not eyeballed) — same
as `cuda_device_chain`'s own result, but this time over five real,
independently-meaningful blocks (including a genuine hash-mixing
function with a real multiply) instead of one trivial increment. nvcc
proved the entire composed specification — both variants, run with
compile-time-known inputs — was a compile-time constant and eliminated
it outright.

## Requirements

Same as `../cuda_device_chain`:

- `nvcc` (part of `nvidia-cuda-toolkit`).
- To actually run it (not just compile): a real CUDA-capable GPU and its
  proprietary driver, `-arch` set to that GPU's real compute capability.
- No PlatformIO integration here, deliberately, same reasoning as
  `cuda_device_chain` — invoke `nvcc` directly as shown above.
