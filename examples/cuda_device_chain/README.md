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

## Real, honest boundary: compiles clean, does not run on this machine

Running the built binary:

```
host: Ticker after 2x inc() = 4 (expect 4)
kernel launch error: no CUDA-capable device is detected
```

The host-side call path (same composed `Ticker` type, no `nvcc`-specific
code) runs correctly. The device-side kernel launch fails at *runtime*,
not compile time — this machine's GPU (GeForce GT 710, Kepler, compute
capability 3.5) isn't even enumerated as CUDA-capable under the CUDA 12.0
toolkit/470.256.02-driver combination installed here. This reinforces,
now at the plain CUDA-runtime level rather than just CUTLASS's own stated
minimum, the same real hardware dead-end already found and documented in
`../cutlass_layout/README.md` (Kepler predates CUTLASS's stated minimum —
Volta — by a full architecture generation).

**Don't conflate the two questions.** "Does `nvcc` accept HAPI's pattern"
— yes, definitively, compile succeeded and the PTX is correct. "Can this
machine run a HAPI-composed device kernel" — no, and that's a hardware
limitation of this specific GPU, unrelated to HAPI, CUDA, or `nvcc`
themselves. `-arch=sm_70` (Volta) was a deliberate virtual-architecture
choice for the compile test — not tied to, and not expected to run on,
this machine's real hardware.

## Requirements

- `nvcc` (part of `nvidia-cuda-toolkit`, `sudo apt install
  nvidia-cuda-toolkit` on Debian/Ubuntu — confirmed real, ~170MB
  installed, no GPU driver required just to compile for a virtual
  architecture).
- No PlatformIO integration here, deliberately — unlike `config_loader`/
  `cutlass_layout`'s `env:native`, `nvcc` isn't a PlatformIO-native
  toolchain the way a host g++/clang target is, and this example needs a
  real local `nvcc` install the same way HLS examples need a real local
  Bambu/Vitis install (not something `lib_deps` can fetch). Invoke `nvcc`
  directly, as shown above.
