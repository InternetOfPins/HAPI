// SPDX-License-Identifier: BSD-3-Clause
//
// Thicker case: a 1-D advection-diffusion operator  A = cd*L + ca*D
// (L = 3-pt Laplacian [1,-2,1], D = central 1st derivative [-1,0,1]),
// i.e. two stacked stateful concerns, each with its own coefficient
// array.
//
//   default   : gko::Combination<double>{cd, Csr(L), ca, Csr(D)} -- the
//               real Ginkgo class for c1*op1 + c2*op2. Per apply(): one
//               full virtual advanced-apply per operator + coefficient
//               LinOps + intermediate cache.
//   -DPREFOLD : what a user writes when L and D need not stay separate --
//               fold into one Csr [cd-ca, -2cd, cd+ca], one SpMV. Still
//               drags Csr's whole internal dispatch stack.
//   -DHAPI    : AdvDiffHapi<double> : gko::LinOp, body composed as
//               hapi::Chain<DiffLayer, AdvLayer>::Part<LinOpTerminal> --
//               each layer contributes its own gko::array as a real
//               Chain<>::Part subobject. One fused single-pass apply_impl,
//               no per-operator virtual, no cache vectors. Still drop-in:
//               same gko::LinOp, same create()/apply().
//
// See ../README.md for the three-way numbers and the fairness note.

#include <chrono>
#include <cstdio>

#include <ginkgo/ginkgo.hpp>
#ifdef HAPI
#include "hapi/hapi.h"
#endif

using V = double;
using vec = gko::matrix::Dense<V>;
using csr = gko::matrix::Csr<V, int>;


// fill a pre-sized tridiagonal Csr from a 3-point stencil {l, c, r}
static void fill_tridiag(csr* m, V l, V c, V r)
{
    const int n = static_cast<int>(m->get_size()[0]);
    auto rp = m->get_row_ptrs();
    auto ci = m->get_col_idxs();
    auto va = m->get_values();
    int pos = 0;
    rp[0] = 0;
    for (int i = 0; i < n; ++i) {
        if (i > 0)     { va[pos] = l; ci[pos] = i - 1; ++pos; }
        {                va[pos] = c; ci[pos] = i;     ++pos; }
        if (i < n - 1) { va[pos] = r; ci[pos] = i + 1; ++pos; }
        rp[i + 1] = pos;
    }
}
static gko::size_type tridiag_nnz(int n) { return n == 1 ? 1 : 3 * n - 2; }


#ifdef HAPI

// ---- two composable layers, each carrying its own coefficient array ----

struct DiffLayer {
    template <typename T>
    struct Part : T {
        gko::array<V> diff_c;
        template <typename... A>
        Part(std::shared_ptr<const gko::Executor> e, A&&... a)
            : T(e, std::forward<A>(a)...), diff_c(e, {1.0, -2.0, 1.0})
        {}
        void diff_contribute(std::size_t n, const V* b, V* x, V s) const
        {
            auto c = diff_c.get_const_data();
            for (std::size_t i = 0; i < n; ++i) {
                V r = c[1] * b[i];
                if (i > 0) r += c[0] * b[i - 1];
                if (i < n - 1) r += c[2] * b[i + 1];
                x[i] += s * r;
            }
        }
    };
};

struct AdvLayer {
    template <typename T>
    struct Part : T {
        gko::array<V> adv_c;
        template <typename... A>
        Part(std::shared_ptr<const gko::Executor> e, A&&... a)
            : T(e, std::forward<A>(a)...), adv_c(e, {-1.0, 0.0, 1.0})
        {}
        void adv_contribute(std::size_t n, const V* b, V* x, V s) const
        {
            auto c = adv_c.get_const_data();
            for (std::size_t i = 0; i < n; ++i) {
                V r = 0;
                if (i > 0) r += c[0] * b[i - 1];
                if (i < n - 1) r += c[2] * b[i + 1];
                x[i] += s * r;
            }
        }
    };
};

struct LinOpTerminal : gko::LinOp {
    LinOpTerminal(std::shared_ptr<const gko::Executor> e, gko::dim<2> s)
        : gko::LinOp(e, s)
    {}
};

template <typename VT>
class AdvDiffHapi
    : public hapi::Chain<DiffLayer, AdvLayer>::template Part<LinOpTerminal>,
      public gko::EnableCreateMethod<AdvDiffHapi<VT>> {
    using ChainBase =
        typename hapi::Chain<DiffLayer, AdvLayer>::template Part<LinOpTerminal>;
    VT cd_, ca_;

public:
    AdvDiffHapi(std::shared_ptr<const gko::Executor> e, gko::size_type n = 0,
                VT cd = 1, VT ca = 1)
        : ChainBase(e, gko::dim<2>{n}), cd_(cd), ca_(ca)
    {}

protected:
    void apply_impl(const gko::LinOp* b, gko::LinOp* x) const override
    {
        auto db = gko::as<vec>(b);
        auto dx = gko::as<vec>(x);
        const auto n = dx->get_size()[0];
        const V* bv = db->get_const_values();
        V* xv = dx->get_values();
        for (std::size_t i = 0; i < n; ++i) xv[i] = 0;
        this->diff_contribute(n, bv, xv, cd_);  // DiffLayer::Part subobject
        this->adv_contribute(n, bv, xv, ca_);   // AdvLayer::Part subobject
    }
    void apply_impl(const gko::LinOp* alpha, const gko::LinOp* b,
                    const gko::LinOp* beta, gko::LinOp* x) const override
    {
        auto dx = gko::as<vec>(x);
        auto tmp = dx->clone();
        this->apply_impl(b, tmp.get());
        dx->scale(beta);
        dx->add_scaled(alpha, tmp);
    }
};

#endif  // HAPI


int main()
{
    const std::size_t n = 32;
    const long iters = 5'000'000;
    const V cd = 1.0, ca = 0.25;
    auto exec = gko::ReferenceExecutor::create();

    auto b = vec::create(exec, gko::dim<2>(n, 1));
    auto x = vec::create(exec, gko::dim<2>(n, 1));
    for (std::size_t i = 0; i < n; ++i)
        b->get_values()[i] = 1.0 / (1.0 + i) + 0.3 * (i % 3);

    std::shared_ptr<gko::LinOp> A;
#if defined(HAPI)
    const char* tag = "hapi     ";
    A = AdvDiffHapi<V>::create(exec, n, cd, ca);
#elif defined(PREFOLD)
    // what a user writes when they DON'T need L and D kept separate:
    // fold cd*L + ca*D into one tridiagonal Csr, one SpMV, one virtual
    // apply_impl. [cd*1+ca*-1, cd*-2+ca*0, cd*1+ca*1] = [cd-ca,-2cd,cd+ca]
    const char* tag = "prefold  ";
    auto F = csr::create(exec, gko::dim<2>(n, n), tridiag_nnz(n));
    fill_tridiag(F.get(), cd - ca, -2.0 * cd, cd + ca);
    A = gko::share(std::move(F));
#else
    const char* tag = "combination";
    auto L = csr::create(exec, gko::dim<2>(n, n), tridiag_nnz(n));
    auto D = csr::create(exec, gko::dim<2>(n, n), tridiag_nnz(n));
    fill_tridiag(L.get(), 1.0, -2.0, 1.0);
    fill_tridiag(D.get(), -1.0, 0.0, 1.0);
    A = gko::Combination<V>::create(
        gko::share(gko::initialize<vec>({cd}, exec)), gko::share(std::move(L)),
        gko::share(gko::initialize<vec>({ca}, exec)), gko::share(std::move(D)));
#endif

    A->apply(b, x);
    V sum = 0;
    for (std::size_t i = 0; i < n; ++i) sum += x->get_values()[i];
    std::printf("%s  x[0..3]= % .6f % .6f % .6f % .6f   sum=% .9f\n", tag,
                x->get_values()[0], x->get_values()[1], x->get_values()[2],
                x->get_values()[3], sum);

    for (int i = 0; i < 1000; ++i) A->apply(b, x);
    auto t0 = std::chrono::steady_clock::now();
    for (long i = 0; i < iters; ++i) A->apply(b, x);
    auto t1 = std::chrono::steady_clock::now();
    std::printf("%s  %8.2f ns/apply\n", tag,
                std::chrono::duration<double, std::nano>(t1 - t0).count() /
                    iters);

#ifdef HAPI
    std::printf("sizeof(AdvDiffHapi<double>) = %zu\n", sizeof(AdvDiffHapi<V>));
#endif
}
