// kdlCompose — KDL's ChainIkSolverPos_NR / _NR_JL with the two sub-solvers as
// compile-time types instead of ChainFkSolverPos& / ChainIkSolverVel& refs.
// Plain C++ templates, not a HAPI composition (see src/kdlSolvers.h and README).
//
// Per family, three implementations compared for bit-exact agreement:
//   shipped   KDL's own class from liborocos-kdl.so
//   Ref       KDL's loop re-expressed here with the abstract-base references
//   T         same loop, sub-solvers as concrete template parameters
//
//   default : PseudoInverse   (KDL ChainIkSolverVel_pinv)
//   -DWDLS  : DampedLeastSquares (KDL ChainIkSolverVel_wdls)

#include <cstddef>
#include <cstdio>

#include <kdl/chain.hpp>
#include <kdl/chainfksolverpos_recursive.hpp>
#include <kdl/chainiksolvervel_pinv.hpp>
#include <kdl/chainiksolvervel_wdls.hpp>
#include <kdl/chainiksolverpos_nr.hpp>
#include <kdl/chainiksolverpos_nr_jl.hpp>

#include "kdlSolvers.h"

#if defined(WDLS)
  using StockVel = KDL::ChainIkSolverVel_wdls;
  static constexpr auto kMethod = kdlCompose::VelMethod::DampedLeastSquares;
  static const char* kTag = "wdls";
#else
  using StockVel = KDL::ChainIkSolverVel_pinv;
  static constexpr auto kMethod = kdlCompose::VelMethod::PseudoInverse;
  static const char* kTag = "pinv";
#endif

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
                       KDL::ChainIkSolverPos& concrete,
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

        KDL::JntArray qinit(nj), qs(nj), qr(nj), qt(nj);
        const int rs = shipped.CartToJnt(qinit, target, qs);
        const int rr = ref.CartToJnt(qinit, target, qr);
        const int rt = concrete.CartToJnt(qinit, target, qt);

        if (rs == rr && rr == rt && same(qs, qr) && same(qr, qt)) {
            std::printf("  %-6s match  seed(% .2f,% .2f,% .2f)  rc=%d (%s)\n",
                        fam, seeds[k][0], seeds[k][1], seeds[k][2], rt, rcName(rt));
        } else {
            ++mismatch;
            std::printf("  %-6s MISMATCH seed(% .2f,% .2f,% .2f)  rc s/r/t=%d/%d/%d\n"
                        "         shipped=(% .12f,% .12f,% .12f)\n"
                        "         ref    =(% .12f,% .12f,% .12f)\n"
                        "         T      =(% .12f,% .12f,% .12f)\n",
                        fam, seeds[k][0], seeds[k][1], seeds[k][2], rs, rr, rt,
                        qs(0), qs(1), qs(2), qr(0), qr(1), qr(2), qt(0), qt(1), qt(2));
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
        KDL::ChainIkSolverPos_NR                shipped(chain, fkS, velS, 100, 1e-6);
        kdlCompose::ChainIkSolverPos_NR_Ref     ref(chain, fkR, velR, 100, 1e-6);
        kdlCompose::ChainIkSolverPos_NR_T<KDL::ChainFkSolverPos_recursive,
                                          kdlCompose::SelectVel<kMethod>> concrete(chain, 100, 1e-6);
        mismatch += checkFamily("NR", chain, shipped, ref, concrete, seeds, nseeds);
    }

    // ── NR_JL, real limits [-2.5, 2.5] rad on every joint ──
    {
        KDL::JntArray lo(nj), hi(nj);
        for (unsigned int j = 0; j < nj; ++j) { lo(j) = -2.5; hi(j) = 2.5; }

        KDL::ChainFkSolverPos_recursive fkS(chain), fkR(chain);
        StockVel velS(chain), velR(chain);
        KDL::ChainIkSolverPos_NR_JL             shipped(chain, lo, hi, fkS, velS, 100, 1e-6);
        kdlCompose::ChainIkSolverPos_NR_JL_Ref  ref(chain, fkR, velR, 100, 1e-6);
        kdlCompose::ChainIkSolverPos_NR_JL_T<KDL::ChainFkSolverPos_recursive,
                                             kdlCompose::SelectVel<kMethod>> concrete(chain, 100, 1e-6);
        ref.setJointLimits(lo, hi);
        concrete.setJointLimits(lo, hi);
        mismatch += checkFamily("NR_JL", chain, shipped, ref, concrete, seeds, nseeds);
    }

    // ── sizeof: one struct with N members vs N separately-allocated objects ──
    std::printf("\n[%s] sizeof (bytes):\n", kTag);
    std::printf("  %-40s %4zu\n", "KDL::ChainIkSolverPos_NR",     sizeof(KDL::ChainIkSolverPos_NR));
    std::printf("  %-40s %4zu\n", "KDL::ChainIkSolverPos_NR_JL",  sizeof(KDL::ChainIkSolverPos_NR_JL));
    std::printf("  %-40s %4zu\n", "  ChainFkSolverPos_recursive", sizeof(KDL::ChainFkSolverPos_recursive));
    std::printf("  %-40s %4zu\n", "  velocity solver",            sizeof(StockVel));
    std::printf("  %-40s %4zu   (NR + fk + vel)\n", "stock NR: three separate objects",
                sizeof(KDL::ChainIkSolverPos_NR) + sizeof(KDL::ChainFkSolverPos_recursive) + sizeof(StockVel));
    std::printf("  %-40s %4zu\n", "ChainIkSolverPos_NR_Ref",
                sizeof(kdlCompose::ChainIkSolverPos_NR_Ref));
    std::printf("  %-40s %4zu\n", "ChainIkSolverPos_NR_T",
                sizeof(kdlCompose::ChainIkSolverPos_NR_T<KDL::ChainFkSolverPos_recursive, StockVel>));
    std::printf("  %-40s %4zu\n", "ChainIkSolverPos_NR_JL_T",
                sizeof(kdlCompose::ChainIkSolverPos_NR_JL_T<KDL::ChainFkSolverPos_recursive, StockVel>));

    std::printf("\n[%s] loopback: %d/%zu mismatch\n", kTag, mismatch, nseeds * 2);
    return mismatch ? 1 : 0;
}
