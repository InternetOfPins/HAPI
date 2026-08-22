/**
 * @file main.cpp
 * @brief Cross-library demonstrator: HAPI + OneData + a real NVIDIA CuTe
 *        (CUTLASS) Layout, host-only, no GPU/nvcc required.
 *
 * A HAPI Chain<> stack wraps a real cute::Layout for coordinate math, with
 * two independent HAPI-composed concerns layered on top: bounds-checking
 * and OneData-backed access counting. This proves HAPI composes a SPATIAL
 * indexing library (CuTe) as cleanly as it already composes TEMPORAL DSP
 * pipelines (OneHLS) or PARSE pipelines (OneParse/config_loader) -- the
 * exact gap the earlier OneHLS x CuTe design-discipline study flagged and
 * never closed (it only ever read CuTe's docs, never compiled real CuTe
 * code). See this example's README for what's deliberately NOT attempted
 * (cutlass::reference::host::Gemm, device kernels) and why.
 *
 * Two real cute::Layout instances share the SAME underlying 4x4 buffer:
 * a plain row-major layout, and a genuine nested/hierarchical CuTe shape
 * ((2,2),(2,2)) -- CuTe's own tiling capability, used for real, not
 * reimplemented. The exact same BoundsCheck/AccessLog HAPI code wraps
 * both, unmodified -- swapping which Layout LayoutIndex<> is instantiated
 * with is the only change needed to switch indexing schemes.
 */

#include <cute/layout.hpp>
#include <hapi/hapi.h>
#include <oneData/oneData.h>
// Deliberately no `using namespace oneData;` -- oneData::Int (=Data<int>)
// collides with cute::Int<v> the moment both are visible unqualified in
// the same scope (the exact feedback_using_namespace_collisions bug
// class, hit for real while building this example). Every oneData::
// reference below stays qualified instead.
#include <iostream>

// ── terminal: raw owned storage ─────────────────────────────────────────

template<int N>
struct RawStore {
  template<typename O> struct Part : O {
    using Base = O; using Base::Base;
    float data[N]{};
    float& raw(int idx) { return data[idx]; }
  };
};

// ── real cute::Layout does the coordinate -> offset math ───────────────

template<auto MakeLayout>
struct LayoutIndex {
  template<typename O> struct Part : O {
    using Base = O; using Base::Base;
    static constexpr auto layout = MakeLayout();
    static int rows() { return cute::size<0>(layout); }
    static int cols() { return cute::size<1>(layout); }
    float& at(int i, int j) { return Base::raw(layout(i, j)); }
  };
};

// ── HAPI layer: bounds-check, independent of which Layout is used ──────

struct BoundsCheck {
  template<typename O> struct Part : O {
    using Base = O; using Base::Base;
    float& at(int i, int j) {
      bool ok = (i >= 0 && i < Base::rows() && j >= 0 && j < Base::cols());
      if (!ok) std::cerr << "  out of bounds: (" << i << "," << j << ")\n";
      return Base::at(ok ? i : 0, ok ? j : 0);
    }
  };
};

// ── HAPI + OneData layer: access-count tracking, also independent ──────

struct AccessLog {
  template<typename O> struct Part : O {
    using Base = O; using Base::Base;
    oneData::DataDef<oneData::Watch<oneData::Int>> count{};
    float& at(int i, int j) {
      count.set(count.get() + 1);
      return Base::at(i, j);
    }
  };
};

// ── two real cute::Layouts over the same 4x4 buffer ─────────────────────

// Layout A: plain row-major.
constexpr auto flatLayoutFn() {
  using namespace cute;
  return make_layout(make_shape(Int<4>{}, Int<4>{}), LayoutRight{});
}

// Layout B: the SAME 16 elements, arranged as 2x2 tiles of 2x2 blocks --
// a genuine nested/hierarchical CuTe Shape/Stride, CuTe's own headline
// tiling capability, not reimplemented.
constexpr auto tiledLayoutFn() {
  using namespace cute;
  return make_layout(
      make_shape(make_shape(Int<2>{}, Int<2>{}), make_shape(Int<2>{}, Int<2>{})),
      make_stride(make_stride(Int<8>{}, Int<1>{}), make_stride(Int<4>{}, Int<2>{})));
}

// RawStore<16> is a real HAPI component (has its own Part<O>), so it
// belongs in the OO... list, terminated by hapi::Nil -- not passed as
// APIOf's terminal-API slot itself (that slot wants a plain, Part<O>-less
// fallback, matching config_loader's ConfigAPI/NameField shape -- a real
// composition-order bug hit while building this example).
using FlatMatrix  = hapi::APIOf<hapi::Nil, AccessLog, BoundsCheck, LayoutIndex<flatLayoutFn>,  RawStore<16>>;
using TiledMatrix = hapi::APIOf<hapi::Nil, AccessLog, BoundsCheck, LayoutIndex<tiledLayoutFn>, RawStore<16>>;

int main() {
  int failures = 0;

  std::cout << "== flat (row-major) layout ==\n";
  FlatMatrix flat;
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) flat.at(i, j) = float(i * 4 + j);
  float v = flat.at(1, 2);
  std::cout << "  flat(1,2) = " << v << "  (expect 6)\n";
  if (v != 6) ++failures;
  std::cout << "  access count = " << flat.count.get() << "  (expect 17)\n";
  flat.at(9, 9); // deliberate out-of-bounds probe -- should warn, not crash
  std::cout << "  access count after OOB probe = " << flat.count.get() << "  (expect 18)\n";

  std::cout << "\n== tiled (nested CuTe Shape) layout, same physical buffer ==\n";
  TiledMatrix tiled;
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) tiled.data[i * 4 + j] = float(i * 4 + j);
  float t00 = tiled.at(0, 0), t11 = tiled.at(1, 1);
  std::cout << "  tiled(0,0) = " << t00 << "  (expect 0)\n";
  std::cout << "  tiled(1,1) = " << t11 << "  (expect 12 -- real CuTe colex "
               "coordinate decomposition into the nested (2,2) shape, hand-"
               "verified against the layout's own strides)\n";
  if (t00 != 0 || t11 != 12) ++failures;
  std::cout << "  access count = " << tiled.count.get() << "  (expect 2 -- "
               "same BoundsCheck/AccessLog HAPI code, unmodified, wrapping "
               "a completely different real CuTe Layout)\n";

  return failures ? 1 : 0;
}
