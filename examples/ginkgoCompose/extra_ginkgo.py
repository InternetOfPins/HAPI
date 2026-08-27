"""
Wires a locally-built reference-only Ginkgo into the native build.

Ginkgo is not a PlatformIO-packaged library and needs a CMake build +
shared-library link, so it goes through env vars rather than lib_deps --
same env-var-gated shape as cutlass_layout/extra_cutlass.py and
OneHLS's extra_hls.py. Omitting the var fails the build with a clear
message; it never silently falls back.

Requires:
  GINKGO_DIR   - path to a Ginkgo checkout (see ./setup_ginkgo.sh, which
                 also builds it reference-only). Defaults to
                 $HOME/ginkgo-src if that exists.

The build tree is expected at $GINKGO_DIR/build with headers in
build/include and the .so files in build/lib.
"""
import os
Import("env")

GK = os.environ.get("GINKGO_DIR") or os.path.expanduser("~/ginkgo-src")
GKB = os.path.join(GK, "build")

if not os.path.isdir(os.path.join(GKB, "lib")):
    print("ginkgoCompose: reference-only Ginkgo not found.")
    print("  expected build at: " + GKB)
    print("  run ./setup_ginkgo.sh (optionally GINKGO_DIR=/path ./setup_ginkgo.sh)")
    env.Exit(1)

libs = ["ginkgo", "ginkgo_reference", "ginkgo_omp", "ginkgo_cuda",
        "ginkgo_hip", "ginkgo_dpcpp", "ginkgo_device"]

env.Append(
    CPPPATH=[os.path.join(GK, "include"), os.path.join(GKB, "include")],
    LIBPATH=[os.path.join(GKB, "lib")],
    LIBS=libs,
    LINKFLAGS=["-Wl,-rpath," + os.path.join(GKB, "lib")],
)
