# kdlCompose

A drop-in [Orocos KDL](https://github.com/orocos/orocos_kinematics_dynamics)
`KDL::ChainIkSolverPos` whose forward-position and inverse-velocity sub-solvers
are resolved at compile time by a HAPI `Chain<>` instead of KDL's runtime
polymorphism — proving HAPI composes a real, non-trivial polymorphic C++
interface (robot kinematics) as cleanly as it already composes sparse linear
algebra (`ginkgoCompose`), DSP pipelines (`OneHLS`), and parse pipelines
(`OneParse`).

`KDL::ChainIkSolverPos_NR` (Newton-Raphson inverse position) and its
joint-limited sibling `_NR_JL` each store two abstract-base references —
`ChainFkSolverPos& fksolver`, `ChainIkSolverVel& iksolver` — and dispatch
through **both on every iteration** of a `maxiter`-bounded loop (default 100).
This example makes that link point compile-time while the class stays a drop-in
`KDL::ChainIkSolverPos`: same `CartToJnt(q_init, p_in, q_out)` call site, no
vtable underneath.

Not reimplemented: the NR math, the FK segment recursion, the pseudo-inverse
SVD. KDL's real `ChainFkSolverPos_recursive` / `ChainIkSolverVel_pinv` /
`ChainIkSolverVel_wdls` are the composed leaves, unmodified.

Host-only (KDL is Eigen-backed). No robot, no URDF — a synthetic 3-DOF planar
arm is enough to exercise the seam.

## Build

```sh
./setup_kdl.sh            # one-time: clones + builds liborocos-kdl (~1 MB) at
                          # ~/kdl, plus header-only Eigen if the system has none
./verify.sh               # plain g++: builds every variant, shows the objdump
                          # fork structure, runs the bit-exact loopback
```

Or with PlatformIO (`KDL_DIR` picked up by `extra_kdl.py`):

```sh
pio run -e pinv          # ChainIkSolverVel_pinv  vs  IkSolverPos_NR_Hapi<PseudoInverse>
pio run -e wdls          # ChainIkSolverVel_wdls  vs  IkSolverPos_NR_Hapi<DampedLeastSquares>
```

## What `src/` contains

| | |
|---|---|
| `src/kdlAPI.h` | `namespace kdlCompose`. `FkStage` / `IkVelStage` / `ClampStage` each wrap one real KDL solver as `Chain<>::Part` subobject state; `HasVelMethod` + `FindFirst<>` pick the velocity solver from a `Chain<pinv, wdls>` pool at compile time; `IkSolverPos_NR_Hapi<Method>` and `IkSolverPos_NR_JL_Hapi<Method>` are the drop-ins — the second is the first plus **one added `Chain` element** (`ClampStage`). |
| `src/kdlCompose.cpp` | `IkSolverPos_NR_Ref` / `_NR_JL_Ref` — KDL's loops re-expressed in-TU with the abstract-base references (the objdump baseline; KDL ships these compiled inside `liborocos-kdl.so`). Then a per-family loopback: 6 targets × {NR, NR_JL} × {KDL shipped, Ref, Hapi}, asserting bit-identical `q_out`. |

The NR loop body is a macro (`KDLCOMPOSE_NR_LOOP` / `KDLCOMPOSE_NR_JL_LOOP`)
shared verbatim between `Ref` and `Hapi`, so the two differ by **exactly** how
the two sub-solver calls are spelled — which is what the objdump measures.

## Result — the objdump (`./verify.sh`)

`CartToJnt` loop body, indirect (vtable) calls:

| | `_NR` | `_NR_JL` |
|---|---|---|
| `..._Ref` (abstract-base refs = KDL's shipped shape) | **2** | **2** |
| `..._Hapi` | **0** | **0** |

In `Hapi` the two dispatches become direct calls to
`KDL::ChainFkSolverPos_recursive::JntToCart` and
`KDL::ChainIkSolverVel_{pinv,wdls}::CartToJnt`. Those concrete bodies stay as
calls into `liborocos-kdl.so` (not inlined — shared-library boundary; LTO or a
header-only KDL would close that too). `_NR_JL`'s per-joint clamp is direct
`JntArray::operator()` calls in both — never a vtable.

**Loopback:** 0/12 mismatch, `q_out` bit-identical across KDL-shipped / Ref /
Hapi, for both `pinv` and `wdls`.

## Honesty notes

- **`sizeof` folds, doesn't collapse.** The composed object embeds the
  sub-solvers instead of pointing at them, so it is larger than the stock NR
  object alone (608–816 B vs 224–256 B) but slightly *smaller* than the three
  separately-allocated stock objects it replaces. The win is dispatch removal +
  one allocation, not footprint — the stages carry real Eigen state, so EBO
  doesn't apply.
- The drop-in matters most for code that instantiates `ChainIkSolverPos_NR`
  directly (KDL's own examples, TRAC-IK internals, FreeCAD's vendored copy).
  MoveIt2 and ros2_control's `kinematics_interface_kdl` tend to hand-roll the NR
  loop rather than use `_NR` — the composition mechanism still applies, but not
  as a literal drop-in there.
- With `wdls` on this arm, plain `_NR` doesn't converge but `_NR_JL` does — the
  joint clamp bounds the iteration. One JL seed still hits `maxiter` (all three
  implementations agree); the clamp is fighting the solution there.
- KDL is **LGPL-2.1-or-later**. This example `#include`s KDL headers and links
  `liborocos-kdl` dynamically; `kdlAPI.h` carries no KDL code, only composition
  over its public interface.

R&D trail: `../../.RnD/kdlCompose/HANDOFF.md`.
