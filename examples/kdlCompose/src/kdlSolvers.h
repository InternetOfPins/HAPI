/**
 * @file kdlSolvers.h
 * KDL::ChainIkSolverPos_NR / _NR_JL with the two sub-solvers as compile-time
 * template parameters instead of ChainFkSolverPos& / ChainIkSolverVel&
 * abstract-base references.
 *
 * This is plain C++ templates — NOT a HAPI composition, and not a HAPI example.
 * KDL's NR position solver has a *closed* seam: exactly one forward-position
 * plus one inverse-velocity sub-solver, the velocity one being a fixed member
 * of KDL's own `ChainIkSolverVel_*` set. Per the collaborator-count filter
 * (a closed set of 2–3 known types), `hapi::Chain<>` / `FindFirst<>` here would
 * be pure ceremony with byte-identical codegen — and the stages carry Eigen
 * state, so there is no empty-base-optimization fold to gain either. What fixing
 * the types buys is exactly what any template does: the per-iteration virtual
 * dispatch goes away (see verify.sh). What it costs: the solver is no longer
 * selectable at runtime. See README.md and ../../.RnD/kdlCompose/HANDOFF.md.
 *
 * Two forms per family, compared by verify.sh's objdump:
 *   ..._Ref   sub-solvers held as ChainFkSolverPos& / ChainIkSolverVel&
 *             (== KDL's shipped ChainIkSolverPos_NR / _NR_JL shape)
 *   ..._T     sub-solvers are concrete template parameters, held by value
 *
 * KDL::Chain names stay fully qualified.
 */
#pragma once

#include <limits>
#include <type_traits>

#include <kdl/chain.hpp>
#include <kdl/frames.hpp>
#include <kdl/jntarray.hpp>
#include <kdl/chainiksolver.hpp>
#include <kdl/chainfksolver.hpp>
#include <kdl/chainfksolverpos_recursive.hpp>
#include <kdl/chainiksolvervel_pinv.hpp>
#include <kdl/chainiksolvervel_wdls.hpp>
#include <kdl/chainiksolverpos_nr.hpp>      // reused for its public E_* codes
#include <kdl/chainiksolverpos_nr_jl.hpp>

namespace kdlCompose {

// A compile-time choice among KDL's fixed velocity-solver set. This is the whole
// of what a `FindFirst<>` "policy" over a 2-element pool amounts to.
enum class VelMethod { PseudoInverse, DampedLeastSquares };

template<VelMethod M>
using SelectVel = std::conditional_t<M == VelMethod::PseudoInverse,
                                     KDL::ChainIkSolverVel_pinv,
                                     KDL::ChainIkSolverVel_wdls>;

// ── the NR position loops ───────────────────────────────────────────────────
// Verbatim from KDL::ChainIkSolverPos_NR::CartToJnt / _NR_JL::CartToJnt,
// parameterised only on how the sub-solver / clamp calls are spelled — so the
// _Ref form (abstract-base references) and the _T form (concrete template
// params) differ by exactly that, which is what verify.sh's objdump measures.
// Expects members chain, nj, maxiter, eps, f, delta_twist, delta_q + statics
// E_FKSOLVERPOS_FAILED / E_IKSOLVER_FAILED (NR) or E_IKSOLVERVEL_FAILED (JL).
// diff / Add / Equal resolve by ADL on the KDL argument types.
#define KDLCOMPOSE_NR_LOOP(FK_JNT_TO_CART, IK_CART_TO_JNT)                    \
    if (nj != chain.getNrOfJoints())                                          \
        return (this->error = KDL::SolverI::E_NOT_UP_TO_DATE);                \
    if (q_init.rows() != nj || q_out.rows() != nj)                            \
        return (this->error = KDL::SolverI::E_SIZE_MISMATCH);                 \
    q_out = q_init;                                                           \
    for (unsigned int _i = 0; _i < maxiter; ++_i) {                          \
        if (KDL::SolverI::E_NOERROR > (FK_JNT_TO_CART))                       \
            return (this->error = E_FKSOLVERPOS_FAILED);                      \
        delta_twist = diff(f, p_in);                                          \
        const int _rc = (IK_CART_TO_JNT);                                     \
        if (KDL::SolverI::E_NOERROR > _rc)                                    \
            return (this->error = E_IKSOLVER_FAILED);                         \
        Add(q_out, delta_q, q_out);                                           \
        if (Equal(delta_twist, KDL::Twist::Zero(), eps))                      \
            return (_rc > KDL::SolverI::E_NOERROR ? KDL::SolverI::E_DEGRADED  \
                                                  : KDL::SolverI::E_NOERROR); \
    }                                                                        \
    return (this->error = KDL::SolverI::E_MAX_ITERATIONS_EXCEEDED);

// JL flavour: convergence tested *before* the velocity solve (via break), each
// joint clamped to limits after the step, no E_DEGRADED. CLAMP_Q_OUT is a
// statement clamping q_out in place; LIMIT_ROWS yields the joint-limit row count.
#define KDLCOMPOSE_NR_JL_LOOP(FK_JNT_TO_CART, IK_CART_TO_JNT, LIMIT_ROWS, CLAMP_Q_OUT) \
    if (nj != chain.getNrOfJoints())                                          \
        return (this->error = KDL::SolverI::E_NOT_UP_TO_DATE);                \
    if (nj != q_init.rows() || nj != q_out.rows() || nj != (LIMIT_ROWS))      \
        return (this->error = KDL::SolverI::E_SIZE_MISMATCH);                 \
    q_out = q_init;                                                           \
    unsigned int _i;                                                          \
    for (_i = 0; _i < maxiter; ++_i) {                                       \
        if ((FK_JNT_TO_CART) < 0)                                             \
            return (this->error = E_FKSOLVERPOS_FAILED);                      \
        delta_twist = diff(f, p_in);                                          \
        if (Equal(delta_twist, KDL::Twist::Zero(), eps))                      \
            break;                                                           \
        if ((IK_CART_TO_JNT) < 0)                                             \
            return (this->error = E_IKSOLVERVEL_FAILED);                      \
        Add(q_out, delta_q, q_out);                                           \
        CLAMP_Q_OUT;                                                          \
    }                                                                        \
    return (this->error = _i != maxiter ? KDL::SolverI::E_NOERROR             \
                                        : KDL::SolverI::E_MAX_ITERATIONS_EXCEEDED);

// ── NR: reference-held (== KDL's shipped shape) ─────────────────────────────
class ChainIkSolverPos_NR_Ref : public KDL::ChainIkSolverPos {
    const KDL::Chain& chain;
    unsigned int nj;
    KDL::ChainFkSolverPos& fk;
    KDL::ChainIkSolverVel& vel;
    KDL::JntArray delta_q;
    KDL::Frame f;
    KDL::Twist delta_twist;
    unsigned int maxiter;
    double eps;

public:
    static constexpr int E_IKSOLVER_FAILED    = KDL::ChainIkSolverPos_NR::E_IKSOLVER_FAILED;
    static constexpr int E_FKSOLVERPOS_FAILED = KDL::ChainIkSolverPos_NR::E_FKSOLVERPOS_FAILED;

    ChainIkSolverPos_NR_Ref(const KDL::Chain& chain, KDL::ChainFkSolverPos& fk,
                            KDL::ChainIkSolverVel& vel, unsigned int maxiter = 100, double eps = 1e-6)
        : chain(chain), nj(chain.getNrOfJoints()), fk(fk), vel(vel),
          delta_q(chain.getNrOfJoints()), maxiter(maxiter), eps(eps) {}

    int CartToJnt(const KDL::JntArray& q_init, const KDL::Frame& p_in, KDL::JntArray& q_out) override
    {
        KDLCOMPOSE_NR_LOOP(fk.JntToCart(q_out, f), vel.CartToJnt(q_out, delta_twist, delta_q))
    }
    void updateInternalDataStructures() override
    {
        nj = chain.getNrOfJoints();
        vel.updateInternalDataStructures();
        fk.updateInternalDataStructures();
        delta_q.resize(nj);
    }
};

// ── NR: concrete sub-solver types ──────────────────────────────────────────
template<class Fk = KDL::ChainFkSolverPos_recursive,
         class Vel = KDL::ChainIkSolverVel_pinv>
class ChainIkSolverPos_NR_T : public KDL::ChainIkSolverPos {
    const KDL::Chain& chain;
    unsigned int nj;
    Fk  fk;
    Vel vel;
    KDL::JntArray delta_q;
    KDL::Frame f;
    KDL::Twist delta_twist;
    unsigned int maxiter;
    double eps;

public:
    static constexpr int E_IKSOLVER_FAILED    = KDL::ChainIkSolverPos_NR::E_IKSOLVER_FAILED;
    static constexpr int E_FKSOLVERPOS_FAILED = KDL::ChainIkSolverPos_NR::E_FKSOLVERPOS_FAILED;

    explicit ChainIkSolverPos_NR_T(const KDL::Chain& chain, unsigned int maxiter = 100, double eps = 1e-6)
        : chain(chain), nj(chain.getNrOfJoints()), fk(chain), vel(chain),
          delta_q(chain.getNrOfJoints()), maxiter(maxiter), eps(eps) {}

    int CartToJnt(const KDL::JntArray& q_init, const KDL::Frame& p_in, KDL::JntArray& q_out) override
    {
        KDLCOMPOSE_NR_LOOP(fk.JntToCart(q_out, f), vel.CartToJnt(q_out, delta_twist, delta_q))
    }
    void updateInternalDataStructures() override
    {
        nj = chain.getNrOfJoints();
        vel.updateInternalDataStructures();
        fk.updateInternalDataStructures();
        delta_q.resize(nj);
    }
};

// ── NR_JL: reference-held ──────────────────────────────────────────────────
class ChainIkSolverPos_NR_JL_Ref : public KDL::ChainIkSolverPos {
    const KDL::Chain& chain;
    unsigned int nj;
    KDL::JntArray q_min, q_max;
    KDL::ChainFkSolverPos& fk;
    KDL::ChainIkSolverVel& vel;
    KDL::JntArray delta_q;
    KDL::Frame f;
    KDL::Twist delta_twist;
    unsigned int maxiter;
    double eps;

public:
    static constexpr int E_IKSOLVERVEL_FAILED = KDL::ChainIkSolverPos_NR_JL::E_IKSOLVERVEL_FAILED;
    static constexpr int E_FKSOLVERPOS_FAILED = KDL::ChainIkSolverPos_NR_JL::E_FKSOLVERPOS_FAILED;

    ChainIkSolverPos_NR_JL_Ref(const KDL::Chain& chain, KDL::ChainFkSolverPos& fk,
                               KDL::ChainIkSolverVel& vel, unsigned int maxiter = 100, double eps = 1e-6)
        : chain(chain), nj(chain.getNrOfJoints()),
          q_min(chain.getNrOfJoints()), q_max(chain.getNrOfJoints()),
          fk(fk), vel(vel), delta_q(chain.getNrOfJoints()), maxiter(maxiter), eps(eps)
    {
        // KDL's own default: min() is the smallest *positive* double, a quirk
        // matched verbatim for a bit-exact no-limits path.
        q_min.data.setConstant(std::numeric_limits<double>::min());
        q_max.data.setConstant(std::numeric_limits<double>::max());
    }

    int setJointLimits(const KDL::JntArray& lo, const KDL::JntArray& hi)
    {
        if (lo.rows() != q_min.rows() || hi.rows() != q_max.rows())
            return (this->error = KDL::SolverI::E_SIZE_MISMATCH);
        q_min = lo; q_max = hi;
        return (this->error = KDL::SolverI::E_NOERROR);
    }
    void clampToLimits(KDL::JntArray& q) const
    {
        for (unsigned int j = 0; j < q_min.rows(); ++j) if (q(j) < q_min(j)) q(j) = q_min(j);
        for (unsigned int j = 0; j < q_max.rows(); ++j) if (q(j) > q_max(j)) q(j) = q_max(j);
    }

    int CartToJnt(const KDL::JntArray& q_init, const KDL::Frame& p_in, KDL::JntArray& q_out) override
    {
        KDLCOMPOSE_NR_JL_LOOP(fk.JntToCart(q_out, f),
                              vel.CartToJnt(q_out, delta_twist, delta_q),
                              q_min.rows(), clampToLimits(q_out))
    }
    void updateInternalDataStructures() override
    {
        nj = chain.getNrOfJoints();
        vel.updateInternalDataStructures();
        fk.updateInternalDataStructures();
        delta_q.resize(nj);
    }
};

// ── NR_JL: concrete sub-solver types ──────────────────────────────────────
template<class Fk = KDL::ChainFkSolverPos_recursive,
         class Vel = KDL::ChainIkSolverVel_pinv>
class ChainIkSolverPos_NR_JL_T : public KDL::ChainIkSolverPos {
    const KDL::Chain& chain;
    unsigned int nj;
    KDL::JntArray q_min, q_max;
    Fk  fk;
    Vel vel;
    KDL::JntArray delta_q;
    KDL::Frame f;
    KDL::Twist delta_twist;
    unsigned int maxiter;
    double eps;

public:
    static constexpr int E_IKSOLVERVEL_FAILED = KDL::ChainIkSolverPos_NR_JL::E_IKSOLVERVEL_FAILED;
    static constexpr int E_FKSOLVERPOS_FAILED = KDL::ChainIkSolverPos_NR_JL::E_FKSOLVERPOS_FAILED;

    explicit ChainIkSolverPos_NR_JL_T(const KDL::Chain& chain, unsigned int maxiter = 100, double eps = 1e-6)
        : chain(chain), nj(chain.getNrOfJoints()),
          q_min(chain.getNrOfJoints()), q_max(chain.getNrOfJoints()),
          fk(chain), vel(chain), delta_q(chain.getNrOfJoints()), maxiter(maxiter), eps(eps)
    {
        q_min.data.setConstant(std::numeric_limits<double>::min());
        q_max.data.setConstant(std::numeric_limits<double>::max());
    }

    int setJointLimits(const KDL::JntArray& lo, const KDL::JntArray& hi)
    {
        if (lo.rows() != q_min.rows() || hi.rows() != q_max.rows())
            return (this->error = KDL::SolverI::E_SIZE_MISMATCH);
        q_min = lo; q_max = hi;
        return (this->error = KDL::SolverI::E_NOERROR);
    }
    void clampToLimits(KDL::JntArray& q) const
    {
        for (unsigned int j = 0; j < q_min.rows(); ++j) if (q(j) < q_min(j)) q(j) = q_min(j);
        for (unsigned int j = 0; j < q_max.rows(); ++j) if (q(j) > q_max(j)) q(j) = q_max(j);
    }

    int CartToJnt(const KDL::JntArray& q_init, const KDL::Frame& p_in, KDL::JntArray& q_out) override
    {
        KDLCOMPOSE_NR_JL_LOOP(fk.JntToCart(q_out, f),
                              vel.CartToJnt(q_out, delta_twist, delta_q),
                              q_min.rows(), clampToLimits(q_out))
    }
    void updateInternalDataStructures() override
    {
        nj = chain.getNrOfJoints();
        vel.updateInternalDataStructures();
        fk.updateInternalDataStructures();
        delta_q.resize(nj);
    }
};

}  // namespace kdlCompose
