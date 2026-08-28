# kdlCompose

**What this shows:** the per-iteration virtual-dispatch cost inside Orocos KDL's
`ChainIkSolverPos_NR` / `_NR_JL`, and what fixing the two sub-solver types at
compile time does to it — plus the trade-off that comes with it.

**What this is not:** a HAPI win. This example was originally written as a HAPI
`Chain<>` composition; on review that framing didn't hold up (see below), and
the code is now plain C++ templates with no HAPI dependency. It stays in the
tree as the honest version of the comparison, paired with `ginkgoCompose`.

## The seam

`KDL::ChainIkSolverPos_NR` (Newton-Raphson inverse position) and its
joint-limited sibling `_NR_JL` each store two references to abstract bases —
`ChainFkSolverPos& fksolver`, `ChainIkSolverVel& iksolver` — and call through
**both on every iteration** of a `maxiter`-bounded loop (default 100):

```cpp
for (i = 0; i < maxiter; i++) {
    fksolver.JntToCart(q_out, f);                  // virtual
    delta_twist = diff(f, p_in);
    iksolver.CartToJnt(q_out, delta_twist, delta_q);  // virtual
    Add(q_out, delta_q, q_out);
    if (Equal(delta_twist, Twist::Zero(), eps)) return ...;
}
```

`src/kdlSolvers.h` re-expresses that loop verbatim (`KDLCOMPOSE_NR_LOOP` /
`KDLCOMPOSE_NR_JL_LOOP` macros) in two forms:

| | sub-solvers held as | |
|---|---|---|
| `ChainIkSolverPos_NR_Ref` | `ChainFkSolverPos&` / `ChainIkSolverVel&` | == KDL's shipped shape |
| `ChainIkSolverPos_NR_T<Fk, Vel>` | concrete template parameters, by value | the compile-time-typed form |

`SelectVel<VelMethod>` is `std::conditional_t` over KDL's fixed velocity-solver
set — that is the whole of what a `FindFirst<>` "policy" over a two-element pool
comes to.

## Build

```sh
./setup_kdl.sh            # one-time: clones + builds liborocos-kdl (~1 MB) at
                          # ~/kdl, plus header-only Eigen if the system has none
./verify.sh               # plain g++: builds both variants, shows the objdump,
                          # runs the bit-exact loopback
```

Or `pio run -e pinv` / `pio run -e wdls` (`KDL_DIR` picked up by `extra_kdl.py`).

## Result — objdump (`./verify.sh`)

`CartToJnt` loop body, indirect (vtable) calls:

| | `_NR` | `_NR_JL` |
|---|---|---|
| `..._Ref` (abstract-base refs) | 2 | 2 |
| `..._T` (concrete types) | **0** | **0** |

In `_T` the two `call *0xNN(%rax)` become direct calls to
`ChainFkSolverPos_recursive::JntToCart` and `ChainIkSolverVel_{pinv,wdls}::CartToJnt`.
Those concrete bodies stay as calls into `liborocos-kdl.so` (shared-library
boundary — LTO or a header-only KDL would close that too). `_NR_JL`'s per-joint
clamp is direct `JntArray::operator()` calls in both — never a vtable.

**This is what any C++ template does.** Fixing a type at compile time removes the
runtime dispatch on it. The earlier `hapi::Chain<FkStage, IkVelStage>::Part<…>`
version of this file compiled `CartToJnt` to the **same instruction sequence with
the same zero indirect calls** — verified by disassembly; it differed only in
member-offset displacements (the `Chain<>::Part` base-subobject fold places the
wrapper's own fields *after* ~400 B of stage state, so it needed 4-byte
displacements where plain members use 1-byte, ending a few bytes *larger* and
reordering fields you did not choose to reorder). The seam is a *closed* set of
two known collaborators, and the stages carry Eigen state, so there is no
empty-base-optimization fold to gain either.

**The trade-off:** `_T` gives up choosing the solver at runtime. The virtual
`ChainFkSolverPos&` / `ChainIkSolverVel&` exists precisely so a caller can swap
implementations without recompiling; `_T` removes that knob. It is a before/after
on *cost*, not on the same capability.

## What is independently real

**Bit-exact fidelity.** The re-expressed loops are checked against KDL's own
shipped `ChainIkSolverPos_NR` / `_NR_JL` on a 3-DOF planar arm, 6 targets ×
{NR, NR_JL} × {shipped, Ref, T}: `q_out` **bit-identical** (`operator==` per
element, no tolerance), **0/12 mismatch**, `pinv` and `wdls`. `_NR_JL` runs with
real limits `[-2.5, 2.5]` rad.

**`sizeof`.** `_T` embeds the sub-solvers instead of pointing at them: 608–816 B
vs 224–256 B for the stock NR object alone, but slightly *under* the three
separately-allocated stock objects it replaces. That is "one struct with three
members vs three `new`s", which plain members give — not a HAPI property.

## Why the original framing failed

The Ginkgo maintainers made the same point about `ginkgoCompose`: an "objdump
shows the vtable gone" result over a closed extension seam only demonstrates that
compile-time type-fixing removes runtime dispatch, which is a fact about C++
templates, not about any composition library. Library extension seams are
*designed* as a few fixed collaborators, so this trap is easy to fall into.
The filter: count the collaborators at the seam and ask whether the set is open
(parts from independent sources, N variable, added without editing existing
code, needing compile-time reordering). A closed set of 2–3 is plain-template
territory. KDL's NR solver seam is a closed set of 2.

R&D trail: `../../.RnD/kdlCompose/HANDOFF.md`.
