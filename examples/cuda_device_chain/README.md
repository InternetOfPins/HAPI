# cuda_device_chain

Does `nvcc` (NVIDIA's real CUDA compiler) accept HAPI's actual, unmodified
`Chain<>`/`APIOf<>` composition — inheritance-fold plus template-template
parameters — and can the composed type run inside a real `__global__`
kernel? A genuinely new toolchain axis for this project family, alongside
HLS (Bambu/Vitis): this is real device-side compilation, not another
host-only demo like `config_loader`/`cutlass_layout`.

**Answer: yes**, confirmed by an actual `nvcc 12.0.140` compile against
`HAPI/include/hapi/hapi.h` exactly as it sits in this repo — zero HAPI
source changes. `HAPI`'s own core has **zero CUDA annotations anywhere**
(confirmed by grep before writing this) — it's pure alias-template and
inheritance plumbing with no executable code of its own, so it needs
none. The requirement falls entirely on whichever *components* you
compose through it: every executable method in `src/main.cu`'s `Counter`/
`DoubleStep` is `__host__ __device__`-annotated, matching real CuTe's own
total-annotation discipline (sampled `cute/layout.hpp`: 118 annotations
across 106 template blocks, zero exceptions found on anything that emits
code).

## Real result: full constant-folding, not just zero calls

```sh
nvcc -std=c++17 -arch=sm_70 -I../../include src/main.cu -o cuda_device_chain
```

compiles clean, zero errors or warnings. The generated PTX for the
`__global__` kernel (`nvcc ... --ptx`) is:

```ptx
.visible .entry _Z6kernelPi(
	.param .u64 _Z6kernelPi_param_0
)
{
	.reg .b32 	%r<2>;
	.reg .b64 	%rd<3>;

	ld.param.u64 	%rd1, [_Z6kernelPi_param_0];
	cvta.to.global.u64 	%rd2, %rd1;
	mov.u32 	%r1, 6;
	st.global.u32 	[%rd2], %r1;
	ret;
}
```

Three `t.inc()` calls (each `DoubleStep::inc()` calling `Counter::inc()`
twice, `+2` per call, `3×2=6`) — the entire `Chain<>`-composed object and
its three method calls disappear at compile time into one `mov.u32 %r1,
6; st.global.u32`. Zero `call`/`.func` instructions anywhere in the whole
`.ptx` file (grepped, not eyeballed) — nvcc didn't just avoid virtual
dispatch, it proved the whole computation was a compile-time constant and
eliminated it entirely. The same zero-overhead property already verified
on AVR/ESP32/ARM disassembly throughout this project, now confirmed on an
NVIDIA PTX target for the first time.

## Real execution, not just a clean compile

Originally documented here as a real, honest boundary: this machine's
original GPU (GeForce GT 710, Kepler, compute capability 3.5) wasn't even
enumerated as CUDA-capable under the CUDA 12.0 toolkit installed here —
the kernel compiled clean and the PTX above was verified correct, but the
launch itself failed at runtime with "no CUDA-capable device is
detected." That boundary is closed now: the machine has a real
CUDA-capable GPU (GeForce GTX 1070, Pascal, compute capability 6.1), and
the exact same, unmodified `src/main.cu` — compiled against the real
architecture this time —

```sh
nvcc -std=c++17 -arch=sm_61 -I../../include src/main.cu -o cuda_device_chain
```

launches and runs correctly:

```
host: Ticker after 2x inc() = 4 (expect 4)
device: Ticker after 3x inc() = 6 (expect 6)
```

Both call paths correct, real device memory (`cudaMalloc`/`cudaMemcpy`),
real kernel launch, exit 0. "Does `nvcc` accept HAPI's pattern" and "can
a HAPI-composed device kernel actually run" are now both yes, not just
the former.

**Related, same hardware**: a real CUTLASS SGEMM
(`cutlass/examples/00_basic_gemm`, NVIDIA's own repo, plain SIMT, not
Tensor-Core) also compiles and runs correctly on this GPU when invoked
directly with `nvcc -arch=sm_61` (bypassing CUTLASS's own CMake, whose
`CUTLASS_NVCC_ARCHS_SUPPORTED` list doesn't include Pascal at all) —
output `Passed.`, checked against an independently-computed reference
kernel. That's a support-*policy* gap in CUTLASS's build system, not a
hard technical wall for every kernel — it won't extend to CUTLASS's
Tensor-Core examples (`07_volta_tensorop_gemm` and later), since Pascal
has no Tensor Core hardware at all, a real gap unrelated to build policy.
See `../cutlass_layout/README.md` for the host-only CuTe `Layout` work
this builds on.

## Requirements

- `nvcc` (part of `nvidia-cuda-toolkit`, `sudo apt install
  nvidia-cuda-toolkit` on Debian/Ubuntu — confirmed real, ~170MB
  installed, no GPU driver required just to compile for a virtual
  architecture like `-arch=sm_70`).
- To actually *run* it (not just compile): a real CUDA-capable GPU and
  its proprietary driver, and `-arch` set to that GPU's real compute
  capability (`sm_61` for the Pascal card this was last verified against
  — check `nvidia-smi`/`ubuntu-drivers devices` for yours).
- No PlatformIO integration here, deliberately — unlike `config_loader`/
  `cutlass_layout`'s `env:native`, `nvcc` isn't a PlatformIO-native
  toolchain the way a host g++/clang target is, and this example needs a
  real local `nvcc` install the same way HLS examples need a real local
  Bambu/Vitis install (not something `lib_deps` can fetch). Invoke `nvcc`
  directly, as shown above.
