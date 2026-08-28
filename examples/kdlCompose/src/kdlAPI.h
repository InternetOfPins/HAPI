/**
 * @file kdlAPI.h
 * kdlCompose — HAPI composition over Orocos KDL's Chain IK/FK solver seam.
 *
 * KDL::ChainIkSolverPos_NR holds two abstract-base references
 * (KDL::ChainFkSolverPos&, KDL::ChainIkSolverVel&) and dispatches through them
 * on every Newton-Raphson iteration. Here that link point is compile-time: the
 * FK and IK-velocity solvers are hapi::Chain<> stages, the velocity solver is
 * picked by a FindFirst<> policy, and the result is still a drop-in
 * KDL::ChainIkSolverPos.
 *
 * Not reimplemented: the NR math, the FK segment recursion, the pseudo-inverse
 * SVD. KDL's real ChainFkSolverPos_recursive / ChainIkSolverVel_pinv /
 * ChainIkSolverVel_wdls are the leaves, unmodified.
 *
 * KDL::Chain and hapi::Chain coexist in this file — every name stays fully
 * qualified, no `using namespace` for either side.
 */
#pragma once

#include <limits>
#include <type_traits>
#include <utility>

#include <kdl/chain.hpp>
#include <kdl/frames.hpp>
#include <kdl/jntarray.hpp>
#include <kdl/chainiksolver.hpp>
#include <kdl/chainfksolver.hpp>
#include <kdl/chainfksolverpos_recursive.hpp>
#include <kdl/chainiksolvervel_pinv.hpp>
#include <kdl/chainiksolvervel_wdls.hpp>
#include <kdl/chainiksolverpos_nr.hpp>   // reused for its public E_* child-failure codes

#include <hapi/chain.h>
#include <hapi/meta.h>

namespace kdlCompose {

// ── velocity-solver selection policy ─────────────────────────────────────────
// The axis along which KDL's ChainIkSolverVel_* implementations differ, lifted
// to a compile-time trait a policy can select on. The primary template is left
// undefined: a solver with no VelTraits is a hard compile error, never a
// silent fallback.
enum class VelMethod { PseudoInverse, DampedLeastSquares };

template<class S> struct VelTraits;
template<> struct VelTraits<KDL::ChainIkSolverVel_pinv> {
  static constexpr VelMethod method = VelMethod::PseudoInverse;
};
template<> struct VelTraits<KDL::ChainIkSolverVel_wdls> {
  static constexpr VelMethod method = VelMethod::DampedLeastSquares;
};

// predicate over a hapi::Chain<>, same protocol as hapi::SameAs and
// ginkgoCompose's ServesExec (Apply / Check / ApplyPack).
template<VelMethod M>
struct HasVelMethod {
  template<class O> using Apply        = std::bool_constant<VelTraits<O>::method == M>;
  template<class O> using Check        = typename hapi::Traverse<HasVelMethod<M>, O>::Beta;
  template<class... OO> using ApplyPack = hapi::Chain<OO...>;
};

// the pool of real KDL velocity solvers a policy may resolve to
using VelPool = hapi::Chain<KDL::ChainIkSolverVel_pinv, KDL::ChainIkSolverVel_wdls>;

// compile-time: VelMethod -> concrete KDL solver type. A method absent from
// VelPool has no ::Result and fails to compile.
template<VelMethod M>
using SelectVel = typename hapi::FindFirst<HasVelMethod<M>>::template Check<VelPool>;

// ── the NR position loops ───────────────────────────────────────────────────
// Verbatim from KDL::ChainIkSolverPos_NR::CartToJnt / ChainIkSolverPos_NR_JL::
// CartToJnt. Parameterised only on how the sub-solver / clamp calls are spelled,
// so the Ref baseline (abstract-base references, in kdlCompose.cpp) and the Hapi
// variant (compile-time stages) differ by exactly that and nothing else — which
// is what verify.sh's objdump measures. Expects members chain, nj, maxiter, eps,
// f, delta_twist, delta_q + statics E_FKSOLVERPOS_FAILED / E_IKSOLVER_FAILED
// (NR) or E_IKSOLVERVEL_FAILED (JL). diff/Add/Equal resolve by ADL.
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

// ── composition stages ──────────────────────────────────────────────────────
// Each stage wraps one real KDL solver as hapi::Chain<>::Part subobject state
// and exposes a distinctly named hook. No name-hiding (the focCompose lesson):
// the drop-in "same call site" is KDL::ChainIkSolverPos::CartToJnt, preserved
// verbatim on the composed class below — not these internal hooks. The
// const KDL::Chain& is threaded through every stage ctor (ginkgoCompose
// Part(ctx, A&&...a) : T(ctx, fwd(a)...) pattern).

struct FkStage {
    template<class T>
    struct Part : T {
        KDL::ChainFkSolverPos_recursive fk;   // was ChainIkSolverPos_NR's ChainFkSolverPos&
        template<class... A>
        explicit Part(const KDL::Chain& chain, A&&... a)
            : T(chain, std::forward<A>(a)...), fk(chain) {}
        int  fkJntToCart(const KDL::JntArray& q_in, KDL::Frame& p_out) { return fk.JntToCart(q_in, p_out); }
        void fkUpdate() { fk.updateInternalDataStructures(); }
    };
};

template<class VelSolver>
struct IkVelStage {
    template<class T>
    struct Part : T {
        VelSolver ikvel;                      // was ChainIkSolverPos_NR's ChainIkSolverVel&
        template<class... A>
        explicit Part(const KDL::Chain& chain, A&&... a)
            : T(chain, std::forward<A>(a)...), ikvel(chain) {}
        int  ikCartToJnt(const KDL::JntArray& q_in, const KDL::Twist& v_in, KDL::JntArray& qdot_out)
            { return ikvel.CartToJnt(q_in, v_in, qdot_out); }
        void ikUpdate() { ikvel.updateInternalDataStructures(); }
    };
};

// joint-limit clamp: the _NR_JL variant's two private JntArrays + inline clamp
// loops, lifted to a composable stage. Default limits match KDL's own choice
// (std::numeric_limits<double>::min() / ::max() — note min() is the smallest
// *positive* double, a KDL quirk kept for bit-exact parity when no limits set).
struct ClampStage {
    template<class T>
    struct Part : T {
        KDL::JntArray q_min, q_max;
        template<class... A>
        explicit Part(const KDL::Chain& chain, A&&... a)
            : T(chain, std::forward<A>(a)...),
              q_min(chain.getNrOfJoints()), q_max(chain.getNrOfJoints())
        {
            q_min.data.setConstant(std::numeric_limits<double>::min());
            q_max.data.setConstant(std::numeric_limits<double>::max());
        }
        unsigned int jointLimitRows() const { return q_min.rows(); }
        int setJointLimits(const KDL::JntArray& lo, const KDL::JntArray& hi)
        {
            if (lo.rows() != q_min.rows() || hi.rows() != q_max.rows())
                return KDL::SolverI::E_SIZE_MISMATCH;
            q_min = lo; q_max = hi;
            return KDL::SolverI::E_NOERROR;
        }
        void clampToLimits(KDL::JntArray& q) const
        {
            for (unsigned int j = 0; j < q_min.rows(); ++j)
                if (q(j) < q_min(j)) q(j) = q_min(j);
            for (unsigned int j = 0; j < q_max.rows(); ++j)
                if (q(j) > q_max(j)) q(j) = q_max(j);
        }
    };
};

// terminal: the KDL abstract base the Chain fold turns into an is-a. Absorbs
// the threaded chain arg; the composed class keeps its own reference.
struct IkPosTerminal : KDL::ChainIkSolverPos {
    explicit IkPosTerminal(const KDL::Chain&) {}
};

// ── the drop-in: KDL::ChainIkSolverPos_NR with the solver link point static ──
template<VelMethod Method = VelMethod::PseudoInverse>
class IkSolverPos_NR_Hapi
    : public hapi::Chain<FkStage, IkVelStage<SelectVel<Method>>>::template Part<IkPosTerminal>
{
    using ChainBase =
        typename hapi::Chain<FkStage, IkVelStage<SelectVel<Method>>>::template Part<IkPosTerminal>;

    const KDL::Chain& chain;
    unsigned int nj;
    KDL::JntArray delta_q;
    KDL::Frame f;
    KDL::Twist delta_twist;
    unsigned int maxiter;
    double eps;

public:
    // KDL::ChainIkSolverPos_NR's own public child-failure codes, reused so they
    // cannot drift (-100 / -101).
    static constexpr int E_IKSOLVER_FAILED    = KDL::ChainIkSolverPos_NR::E_IKSOLVER_FAILED;
    static constexpr int E_FKSOLVERPOS_FAILED = KDL::ChainIkSolverPos_NR::E_FKSOLVERPOS_FAILED;

    explicit IkSolverPos_NR_Hapi(const KDL::Chain& chain,
                                 unsigned int maxiter = 100, double eps = 1e-6)
        : ChainBase(chain),
          chain(chain),
          nj(chain.getNrOfJoints()),
          delta_q(chain.getNrOfJoints()),
          maxiter(maxiter), eps(eps) {}

    // KDL::ChainIkSolverPos::CartToJnt — the preserved call site.
    int CartToJnt(const KDL::JntArray& q_init, const KDL::Frame& p_in,
                  KDL::JntArray& q_out) override
    {
        KDLCOMPOSE_NR_LOOP(this->fkJntToCart(q_out, f),
                           this->ikCartToJnt(q_out, delta_twist, delta_q))
    }

    void updateInternalDataStructures() override
    {
        nj = chain.getNrOfJoints();
        this->ikUpdate();
        this->fkUpdate();
        delta_q.resize(nj);
    }
};

// ── same, + joint limits: KDL::ChainIkSolverPos_NR_JL, clamp is a third stage ─
// The ONLY structural difference from IkSolverPos_NR_Hapi is one added Chain
// element (ClampStage) and the JL loop flavour. Cf. KDL, where _NR and _NR_JL
// are two separately hand-written classes with duplicated members + loop.
template<VelMethod Method = VelMethod::PseudoInverse>
class IkSolverPos_NR_JL_Hapi
    : public hapi::Chain<FkStage, IkVelStage<SelectVel<Method>>, ClampStage>::template Part<IkPosTerminal>
{
    using ChainBase =
        typename hapi::Chain<FkStage, IkVelStage<SelectVel<Method>>, ClampStage>::template Part<IkPosTerminal>;

    const KDL::Chain& chain;
    unsigned int nj;
    KDL::JntArray delta_q;
    KDL::Frame f;
    KDL::Twist delta_twist;
    unsigned int maxiter;
    double eps;

public:
    // KDL::ChainIkSolverPos_NR_JL's own public child-failure codes
    static constexpr int E_IKSOLVERVEL_FAILED = KDL::ChainIkSolverPos_NR_JL::E_IKSOLVERVEL_FAILED;
    static constexpr int E_FKSOLVERPOS_FAILED = KDL::ChainIkSolverPos_NR_JL::E_FKSOLVERPOS_FAILED;

    explicit IkSolverPos_NR_JL_Hapi(const KDL::Chain& chain,
                                    unsigned int maxiter = 100, double eps = 1e-6)
        : ChainBase(chain), chain(chain), nj(chain.getNrOfJoints()),
          delta_q(chain.getNrOfJoints()), maxiter(maxiter), eps(eps) {}

    int CartToJnt(const KDL::JntArray& q_init, const KDL::Frame& p_in,
                  KDL::JntArray& q_out) override
    {
        KDLCOMPOSE_NR_JL_LOOP(this->fkJntToCart(q_out, f),
                              this->ikCartToJnt(q_out, delta_twist, delta_q),
                              this->jointLimitRows(),
                              this->clampToLimits(q_out))
    }

    // setJointLimits(lo, hi) is inherited from ClampStage unhidden — same
    // signature as KDL::ChainIkSolverPos_NR_JL::setJointLimits.

    void updateInternalDataStructures() override
    {
        nj = chain.getNrOfJoints();
        this->ikUpdate();
        this->fkUpdate();
        delta_q.resize(nj);
    }
};

}  // namespace kdlCompose
