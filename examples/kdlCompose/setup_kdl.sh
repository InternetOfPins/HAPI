#!/usr/bin/env bash
# One-time: clone + build + install a local Orocos KDL (liborocos-kdl) for this
# example, plus a header-only Eigen if the system has none. KDL's C++ core is
# ~1 MB, a few seconds at -j4. Re-runnable (skips clone/configure if present).
#
#   ./setup_kdl.sh                    -> ~/kdl
#   KDL_DIR=/big/disk/kdl ./setup_kdl.sh
#
# Layout produced under $KDL_DIR:
#   src/       git checkout (orocos_kinematics_dynamics)
#   eigen/     Eigen 3.4.0 headers  (only if no system Eigen3)
#   build/     cmake build tree
#   install/   install prefix -> install/include/kdl, install/lib/liborocos-kdl.so
#
# Then: ./verify.sh   (plain g++ reproducer), or pio run -e pinv
set -euo pipefail

KDL_DIR=${KDL_DIR:-$HOME/kdl}
SRC=$KDL_DIR/src
BUILD=$KDL_DIR/build
INSTALL=$KDL_DIR/install
EIGEN_LOCAL=$KDL_DIR/eigen

mkdir -p "$KDL_DIR"

# --- Eigen3 (header-only) ---------------------------------------------------
EIGEN_ARG=()
if pkg-config --exists eigen3 2>/dev/null; then
    echo "setup_kdl: using system Eigen3 ($(pkg-config --modversion eigen3))"
elif [ -f /usr/include/eigen3/Eigen/Core ]; then
    echo "setup_kdl: using system Eigen3 (/usr/include/eigen3)"
    EIGEN_ARG=(-DEIGEN3_INCLUDE_DIR=/usr/include/eigen3)
else
    if [ ! -f "$EIGEN_LOCAL/Eigen/Core" ]; then
        echo "setup_kdl: no system Eigen3 -- fetching Eigen 3.4.0 headers"
        curl -sL -o "$KDL_DIR/eigen.tar.gz" \
            https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
        tar xzf "$KDL_DIR/eigen.tar.gz" -C "$KDL_DIR"
        mv "$KDL_DIR/eigen-3.4.0" "$EIGEN_LOCAL"
        rm -f "$KDL_DIR/eigen.tar.gz"
    fi
    EIGEN_ARG=(-DEIGEN3_INCLUDE_DIR="$EIGEN_LOCAL")
    echo "setup_kdl: using local Eigen at $EIGEN_LOCAL"
fi

# --- KDL -------------------------------------------------------------------
[ -d "$SRC/.git" ] || git clone --depth 1 \
    https://github.com/orocos/orocos_kinematics_dynamics.git "$SRC"

# the C++ library is the orocos_kdl/ subdirectory (its own CMake project)
[ -f "$BUILD/CMakeCache.txt" ] || cmake -S "$SRC/orocos_kdl" -B "$BUILD" \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DENABLE_TESTS=OFF \
    "${EIGEN_ARG[@]}"

cmake --build "$BUILD" -j"${JOBS:-4}"
cmake --install "$BUILD" --prefix "$INSTALL"

echo
echo "done: KDL_DIR=$KDL_DIR"
echo "  headers : $INSTALL/include/kdl/"
echo "  library : $INSTALL/lib/"
[ ${#EIGEN_ARG[@]} -gt 0 ] && echo "  eigen   : ${EIGEN_ARG[0]#-DEIGEN3_INCLUDE_DIR=}"
echo "set KDL_DIR=$KDL_DIR for ./verify.sh and extra_kdl.py if not the default."
