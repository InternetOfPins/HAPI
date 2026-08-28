// kdlCompose — KDL's runtime IK/FK sub-solver link point made compile-time,
// still a drop-in KDL::ChainIkSolverPos. Two solver families:
//
//   ..._NR      KDL::ChainIkSolverPos_NR      (Newton-Raphson)
//   ..._NR_JL   KDL::ChainIkSolverPos_NR_JL   (+ per-joint limit clamp)
//
// and, per family, three implementations compared for bit-exact agreement:
//
//   shipped   KDL's own class from liborocos-kdl.so
//   Ref       KDL's loop re-expressed in THIS TU with ChainFkSolverPos& /
//             ChainIkSolverVel& abstract-base refs (== shipped's shape) —
//             the objdump baseline
//   Hapi      same loop (KDLCOMPOSE_NR*_LOOP), sub-solvers are hapi::Chain<>
//             stages, velocity solver chosen by FindFirst<> at compile time.
//             _NR_JL differs from _NR by exactly one added Chain element.
//
//   default : PseudoInverse   (vs KDL ChainIkSolverVel_pinv)
//   -DWDLS  : DampedLeastSquares (vs KDL ChainIkSolverVel_wdls)

#include <cstddef>
#include <cstdio>

#include <kdl/chain.hpp>
#include <kdl/chainfksolverpos_recursive.hpp>
#include <kdl/chainiksolvervel_pinv.hpp>
#include <kdl/chainiksolvervel_wdls.hpp>
#include <kdl/chainiksolverpos_nr.hpp>
#include <kdl/chainiksolverpos_nr_jl.hpp>

#include "kdlAPI.h"

#if defined(WDLS)
  using StockVel = KDL::ChainIkSolverVel_wdls;
  static constexpr auto kMethod = kdlCompose::VelMethod::DampedLeastSquares;
  static const char* kTag = "wdls";
#else
  using StockVel = KDL::ChainIkSolverVel_pinv;
  static constexpr auto kMethod = kdlCompose::VelMethod::PseudoInverse;
  static const char* kTag = "pinv";
#endif

namespace kdlCompose {

// KDL::ChainIkSolverPos_NR logic re-expressed in-TU so its loop is objdump-
// visible (KDL ships it inside liborocos-kdl.so). Sub-solvers held exactly as
// the shipped class holds them: abstract-base references.
class IkSolverPos_NR_Ref : public KDL::ChainIkSolverPos {
    const KDL::Chain& chain;
    unsigned int nj;
    KDL::ChainFkSolverPos& fksolver;
    KDL::ChainIkSolverVel& iksolver;
    KDL::JntArray delta_q;
    KDL::Frame f;
    KDL::Twist delta_twist;
    unsigned int maxiter;
    double eps;

public:
    static constexpr int E_IKSOLVER_FAILED    = KDL::ChainIkSolverPos_NR::E_IKSOLVER_FAILED;
    static constexpr int E_FKSOLVERPOS_FAILED = KDL::ChainIkSolverPos_NR::E_FKSOLVERPOS_FAILED;

    IkSolverPos_NR_Ref(const KDL::Chain& chain, KDL::ChainFkSolverPos& fk,
                       KDL::ChainIkSolverVel& ik, unsigned int maxiter = 100, double eps = 1e-6)
        : chain(chain), nj(chain.getNrOfJoints()), fksolver(fk), iksolver(ik),
          delta_q(chain.getNrOfJoints()), maxiter(maxiter), eps(eps) {}

    int CartToJnt(const KDL::JntArray& q_init, const KDL::Frame& p_in, KDL::JntArray& q_out) override
    {
        KDLCOMPOSE_NR_LOOP(fksolver.JntToCart(q_out, f),
                           iksolver.CartToJnt(q_out, delta_twist, delta_q))
    }
    void updateInternalDataStructures() override
    {
        nj = chain.getNrOfJoints();
        iksolver.updateInternalDataStructures();
        fksolver.updateInternalDataStructures();
        delta_q.resize(nj);
    }
};

// Same for KDL::ChainIkSolverPos_NR_JL: refs + inline clamp + the two limit
// JntArrays as plain members (exactly the shipped class's shape).
class IkSolverPos_NR_JL_Ref : public KDL::ChainIkSolverPos {
    const KDL::Chain& chain;
    unsigned int nj;
    KDL::JntArray q_min, q_max;
    KDL::ChainFkSolverPos& fksolver;
    KDL::ChainIkSolverVel& iksolver;
    KDL::JntArray delta_q;
    KDL::Frame f;
    KDL::Twist delta_twist;
    unsigned int maxiter;
    double eps;

public:
    static constexpr int E_IKSOLVERVEL_FAILED = KDL::ChainIkSolverPos_NR_JL::E_IKSOLVERVEL_FAILED;
    static constexpr int E_FKSOLVERPOS_FAILED = KDL::ChainIkSolverPos_NR_JL::E_FKSOLVERPOS_FAILED;

    IkSolverPos_NR_JL_Ref(const KDL::Chain& chain, KDL::ChainFkSolverPos& fk,
                          KDL::ChainIkSolverVel& ik, unsigned int maxiter = 100, double eps = 1e-6)
        : chain(chain), nj(chain.getNrOfJoints()),
          q_min(chain.getNrOfJoints()), q_max(chain.getNrOfJoints()),
          fksolver(fk), iksolver(ik), delta_q(chain.getNrOfJoints()),
          maxiter(maxiter), eps(eps)
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
        KDLCOMPOSE_NR_JL_LOOP(fksolver.JntToCart(q_out, f),
                              iksolver.CartToJnt(q_out, delta_twist, delta_q),
                              q_min.rows(),
                              clampToLimits(q_out))
    }
    void updateInternalDataStructures() override
    {
        nj = chain.getNrOfJoints();
        iksolver.updateInternalDataStructures();
        fksolver.updateInternalDataStructures();
        delta_q.resize(nj);
    }
};

}  // namespace kdlCompose

// ── harness ─────────────────────────────────────────────────────────────────

static KDL::Chain rrr(double L = 1.0)
{
    KDL::Chain c;
    for (int i = 0; i < 3; ++i)
        c.addSegment(KDL::Segment(KDL::Joint(KDL::Joint::RotZ), KDL::Frame(KDL::Vector(L, 0, 0))));
    return c;
}

static bool same(const KDL::JntArray& a, const KDL::JntArray& b)
{
    if (a.rows() != b.rows()) return false;
    for (unsigned int i = 0; i < a.rows(); ++i)
        if (a(i) != b(i)) return false;   // bit-exact: same code path, no tolerance
    return true;
}

static const char* rcName(int rc)
{
    switch (rc) {
        case KDL::SolverI::E_NOERROR:                 return "converged";
        case KDL::SolverI::E_DEGRADED:                return "converged/degraded";
        case KDL::SolverI::E_MAX_ITERATIONS_EXCEEDED: return "not converged";
        default:                                      return "error";
    }
}

// solve `target` from zero with all three and require bit-exact agreement
static int checkFamily(const char* fam, const KDL::Chain& chain,
                       KDL::ChainIkSolverPos& shipped, KDL::ChainIkSolverPos& ref,
                       KDL::ChainIkSolverPos& hapi,
                       const double seeds[][3], std::size_t nseeds)
{
    const unsigned int nj = chain.getNrOfJoints();
    KDL::ChainFkSolverPos_recursive fk(chain);
    int mismatch = 0;
    for (std::size_t k = 0; k < nseeds; ++k) {
        KDL::JntArray qgoal(nj);
        for (unsigned int i = 0; i < nj; ++i) qgoal(i) = seeds[k][i];
        KDL::Frame target;
        fk.JntToCart(qgoal, target);

        KDL::JntArray qinit(nj), qs(nj), qr(nj), qh(nj);
        const int rs = shipped.CartToJnt(qinit, target, qs);
        const int rr = ref.CartToJnt(qinit, target, qr);
        const int rh = hapi.CartToJnt(qinit, target, qh);

        if (rs == rr && rr == rh && same(qs, qr) && same(qr, qh)) {
            std::printf("  %-6s match  seed(% .2f,% .2f,% .2f)  rc=%d (%s)\n",
                        fam, seeds[k][0], seeds[k][1], seeds[k][2], rh, rcName(rh));
        } else {
            ++mismatch;
            std::printf("  %-6s MISMATCH seed(% .2f,% .2f,% .2f)  rc s/r/h=%d/%d/%d\n"
                        "         shipped=(% .12f,% .12f,% .12f)\n"
                        "         ref    =(% .12f,% .12f,% .12f)\n"
                        "         hapi   =(% .12f,% .12f,% .12f)\n",
                        fam, seeds[k][0], seeds[k][1], seeds[k][2], rs, rr, rh,
                        qs(0), qs(1), qs(2), qr(0), qr(1), qr(2), qh(0), qh(1), qh(2));
        }
    }
    return mismatch;
}

int main()
{
    const KDL::Chain chain = rrr();
    const unsigned int nj = chain.getNrOfJoints();

    const double seeds[][3] = {
        { 0.10,  0.20,  0.30 }, { 0.50, -0.30,  0.20 }, { -0.40, 0.60, -0.20 },
        { 0.00,  1.00,  0.00 }, { 0.80,  0.10, -0.50 }, {  2.00, -2.00,  2.00 },
    };
    const std::size_t nseeds = sizeof(seeds) / sizeof(seeds[0]);
    int mismatch = 0;

    // ── NR ──
    {
        KDL::ChainFkSolverPos_recursive fkS(chain), fkR(chain);
        StockVel velS(chain), velR(chain);
        KDL::ChainIkSolverPos_NR         shipped(chain, fkS, velS, 100, 1e-6);
        kdlCompose::IkSolverPos_NR_Ref   ref(chain, fkR, velR, 100, 1e-6);
        kdlCompose::IkSolverPos_NR_Hapi<kMethod> hapi(chain, 100, 1e-6);
        mismatch += checkFamily("NR", chain, shipped, ref, hapi, seeds, nseeds);
    }

    // ── NR_JL, real limits [-2.5, 2.5] rad on every joint ──
    {
        KDL::JntArray lo(nj), hi(nj);
        for (unsigned int j = 0; j < nj; ++j) { lo(j) = -2.5; hi(j) = 2.5; }

        KDL::ChainFkSolverPos_recursive fkS(chain), fkR(chain);
        StockVel velS(chain), velR(chain);
        KDL::ChainIkSolverPos_NR_JL        shipped(chain, lo, hi, fkS, velS, 100, 1e-6);
        kdlCompose::IkSolverPos_NR_JL_Ref  ref(chain, fkR, velR, 100, 1e-6);
        kdlCompose::IkSolverPos_NR_JL_Hapi<kMethod> hapi(chain, 100, 1e-6);
        ref.setJointLimits(lo, hi);
        hapi.setJointLimits(lo, hi);
        mismatch += checkFamily("NR_JL", chain, shipped, ref, hapi, seeds, nseeds);
    }

    // ── sizeof ──
    std::printf("\n[%s] sizeof (bytes):\n", kTag);
    std::printf("  %-38s %4zu\n", "KDL::ChainIkSolverPos_NR",     sizeof(KDL::ChainIkSolverPos_NR));
    std::printf("  %-38s %4zu\n", "KDL::ChainIkSolverPos_NR_JL",  sizeof(KDL::ChainIkSolverPos_NR_JL));
    std::printf("  %-38s %4zu\n", "  ChainFkSolverPos_recursive", sizeof(KDL::ChainFkSolverPos_recursive));
    std::printf("  %-38s %4zu\n", "  velocity solver",            sizeof(StockVel));
    std::printf("  %-38s %4zu   (NR + fk + vel)\n", "stock NR stack, 3 objects",
                sizeof(KDL::ChainIkSolverPos_NR) + sizeof(KDL::ChainFkSolverPos_recursive) + sizeof(StockVel));
    std::printf("  %-38s %4zu\n", "IkSolverPos_NR_Ref",           sizeof(kdlCompose::IkSolverPos_NR_Ref));
    std::printf("  %-38s %4zu\n", "IkSolverPos_NR_Hapi",          sizeof(kdlCompose::IkSolverPos_NR_Hapi<kMethod>));
    std::printf("  %-38s %4zu\n", "IkSolverPos_NR_JL_Ref",        sizeof(kdlCompose::IkSolverPos_NR_JL_Ref));
    std::printf("  %-38s %4zu\n", "IkSolverPos_NR_JL_Hapi",       sizeof(kdlCompose::IkSolverPos_NR_JL_Hapi<kMethod>));

    std::printf("\n[%s] loopback: %d/%zu mismatch\n", kTag, mismatch, nseeds * 2);
    return mismatch ? 1 : 0;
}
