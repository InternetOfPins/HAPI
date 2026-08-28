"""
Wires a locally-built Orocos KDL (liborocos-kdl) into the native build.

KDL is not a PlatformIO-packaged library and needs a CMake build + shared-
library link, so it goes through env vars rather than lib_deps -- same env-
var-gated shape as ginkgoCompose/extra_ginkgo.py. Omitting the var fails the
build with a clear message; it never silently falls back.

Requires:
  KDL_DIR   - path produced by ./setup_kdl.sh (default: ~/kdl). Expected:
              $KDL_DIR/install/include/kdl/*.hpp  and
              $KDL_DIR/install/lib/liborocos-kdl.so
  EIGEN_DIR - optional; Eigen include root. If unset: $KDL_DIR/eigen, then
              /usr/include/eigen3, then pkg-config eigen3.
"""
import os
import subprocess

Import("env")

KDL = os.environ.get("KDL_DIR") or os.path.expanduser("~/kdl")
INSTALL = os.path.join(KDL, "install")

if not os.path.isfile(os.path.join(INSTALL, "lib", "liborocos-kdl.so")):
    print("kdlCompose: local KDL not found.")
    print("  expected: " + os.path.join(INSTALL, "lib", "liborocos-kdl.so"))
    print("  run ./setup_kdl.sh  (optionally KDL_DIR=/path ./setup_kdl.sh)")
    env.Exit(1)


def eigen_include():
    if os.environ.get("EIGEN_DIR"):
        return os.environ["EIGEN_DIR"]
    local = os.path.join(KDL, "eigen")
    if os.path.isfile(os.path.join(local, "Eigen", "Core")):
        return local
    if os.path.isfile("/usr/include/eigen3/Eigen/Core"):
        return "/usr/include/eigen3"
    try:
        out = subprocess.check_output(["pkg-config", "--cflags-only-I", "eigen3"])
        return out.decode().strip().lstrip("-I") or None
    except Exception:
        return None


eig = eigen_include()
if not eig:
    print("kdlCompose: Eigen3 headers not found -- set EIGEN_DIR or re-run ./setup_kdl.sh")
    env.Exit(1)

libdir = os.path.join(INSTALL, "lib")
env.Append(
    CPPPATH=[os.path.join(INSTALL, "include"), eig],
    LIBPATH=[libdir],
    LIBS=["orocos-kdl"],
    LINKFLAGS=["-Wl,-rpath," + libdir],
    CCFLAGS=["-Wno-deprecated-copy"],  # KDL 1.5.4 headers, not our code
)
