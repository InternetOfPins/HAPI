// SPDX-License-Identifier: BSD-3-Clause
//
// Round 2: Ginkgo's own examples/custom-matrix-format/ (StencilMatrix,
// the TOMS-2022 paper's Listing 7), with the composition layer rewritten
// so the executor and kernel-variant choice are compile-time -- while the
// class stays a drop-in gko::LinOp.
//
//   default   : StencilMatrix -- verbatim from Ginkgo's example, trimmed
//               to the reference executor. apply() traverses all 3
//               polymorphism forks (format / executor / kernel variant,
//               paper section 7.2).
//   -DHAPI    : StencilMatrixHapi -- gko::Operation's per-executor run()
//               overload set becomes hapi::Chain<RefStencil,OmpStencil>,
//               selected by hapi::FindFirst<ServesExec<Exec>> at compile
//               time; apply_impl calls the chosen kernel directly. Still
//               : public gko::LinOp, still create(), still drops into
//               cg::build()...->generate(...)->apply(...).
//
// Prints: the 1-D Poisson solve error (correctness) and a bare-apply()
// timing loop (paper section 7.2 methodology, size-11 matrix).
//
// See ../README.md for the objdump comparison -- the load-bearing result.

#include <chrono>
#include <cstdio>

#include <ginkgo/ginkgo.hpp>
#ifdef HAPI
#include "hapi/hapi.h"
#endif

using ValueType = double;
using vec = gko::matrix::Dense<ValueType>;


#ifdef HAPI

// --- backend kernel set: declared once, same role as gko::Operation ---
struct RefStencil {
    using exec_type = gko::ReferenceExecutor;
    template <typename V>
    static void run(std::size_t n, const V* c, const V* b, V* x)
    {
        for (std::size_t i = 0; i < n; ++i) {
            auto r = c[1] * b[i];
            if (i > 0) r += c[0] * b[i - 1];
            if (i < n - 1) r += c[2] * b[i + 1];
            x[i] = r;
        }
    }
};
struct OmpStencil {
    using exec_type = gko::OmpExecutor;
    template <typename V>
    static void run(std::size_t n, const V* c, const V* b, V* x)
    {
#pragma omp parallel for
        for (std::size_t i = 0; i < n; ++i) {
            auto r = c[1] * b[i];
            if (i > 0) r += c[0] * b[i - 1];
            if (i < n - 1) r += c[2] * b[i + 1];
            x[i] = r;
        }
    }
};

// predicate over a Chain, same protocol as hapi::SameAs / hapi::TagIs
template <typename E>
struct ServesExec {
    template <typename O>
    using Check = typename hapi::Traverse<ServesExec<E>, O>::Beta;
    template <typename O>
    using Apply = std::is_same<E, typename O::exec_type>;
    template <typename... OO>
    using ApplyPack = hapi::Chain<OO...>;
};

template <typename VT, typename Exec,
          typename Kernels = hapi::Chain<RefStencil, OmpStencil>>
class StencilMatrixHapi
    : public gko::LinOp,
      public gko::EnableCreateMethod<StencilMatrixHapi<VT, Exec, Kernels>> {
public:
    StencilMatrixHapi(std::shared_ptr<const gko::Executor> exec,
                      gko::size_type size = 0, VT left = -1.0, VT center = 2.0,
                      VT right = -1.0)
        : gko::LinOp(exec, gko::dim<2>{size}),
          coefficients(exec, {left, center, right})
    {}

protected:
    using coef_type = gko::array<VT>;
    using Kernel =
        typename hapi::FindFirst<ServesExec<Exec>>::template Check<Kernels>;

    void apply_impl(const gko::LinOp* b, gko::LinOp* x) const override
    {
        auto dense_b = gko::as<vec>(b);
        auto dense_x = gko::as<vec>(x);
        Kernel::run(dense_x->get_size()[0], coefficients.get_const_data(),
                    dense_b->get_const_values(), dense_x->get_values());
    }
    void apply_impl(const gko::LinOp* alpha, const gko::LinOp* b,
                    const gko::LinOp* beta, gko::LinOp* x) const override
    {
        auto dense_x = gko::as<vec>(x);
        auto tmp = dense_x->clone();
        this->apply_impl(b, tmp.get());
        dense_x->scale(beta);
        dense_x->add_scaled(alpha, tmp);
    }

private:
    coef_type coefficients;
};

using Stencil = StencilMatrixHapi<ValueType, gko::ReferenceExecutor>;
static const char* kTag = "hapi    ";

#else  // baseline: Ginkgo's own example (reference-executor trim)

template <typename VT>
class StencilMatrix : public gko::LinOp,
                      public gko::EnableCreateMethod<StencilMatrix<VT>> {
public:
    StencilMatrix(std::shared_ptr<const gko::Executor> exec,
                  gko::size_type size = 0, VT left = -1.0, VT center = 2.0,
                  VT right = -1.0)
        : gko::LinOp(exec, gko::dim<2>{size}),
          coefficients(exec, {left, center, right})
    {}

protected:
    using coef_type = gko::array<VT>;

    void apply_impl(const gko::LinOp* b, gko::LinOp* x) const override
    {
        auto dense_b = gko::as<vec>(b);
        auto dense_x = gko::as<vec>(x);
        struct stencil_operation : gko::Operation {
            stencil_operation(const coef_type& c, const vec* b, vec* x)
                : coefficients{c}, b{b}, x{x}
            {}
            void run(std::shared_ptr<const gko::OmpExecutor>) const override
            {
                auto bv = b->get_const_values();
                auto xv = x->get_values();
                for (std::size_t i = 0; i < x->get_size()[0]; ++i) {
                    auto c = coefficients.get_const_data();
                    auto r = c[1] * bv[i];
                    if (i > 0) r += c[0] * bv[i - 1];
                    if (i < x->get_size()[0] - 1) r += c[2] * bv[i + 1];
                    xv[i] = r;
                }
            }
            const coef_type& coefficients;
            const vec* b;
            vec* x;
        };
        this->get_executor()->run(
            stencil_operation(coefficients, dense_b, dense_x));
    }
    void apply_impl(const gko::LinOp* alpha, const gko::LinOp* b,
                    const gko::LinOp* beta, gko::LinOp* x) const override
    {
        auto dense_x = gko::as<vec>(x);
        auto tmp = dense_x->clone();
        this->apply_impl(b, tmp.get());
        dense_x->scale(beta);
        dense_x->add_scaled(alpha, tmp);
    }

private:
    coef_type coefficients;
};

using Stencil = StencilMatrix<ValueType>;
static const char* kTag = "baseline";

#endif


template <typename Closure, typename VT>
static void generate_rhs(Closure f, VT u0, VT u1, vec* rhs)
{
    const auto n = rhs->get_size()[0];
    auto v = rhs->get_values();
    const VT h = 1.0 / (n + 1);
    for (std::size_t i = 0; i < n; ++i) v[i] = -f(VT(i + 1) * h) * h * h;
    v[0] += u0;
    v[n - 1] += u1;
}

template <typename Closure>
static double rel_error(int n, const vec* u, Closure correct_u)
{
    const auto h = 1.0 / (n + 1);
    double e = 0;
    for (int i = 0; i < n; ++i) {
        using std::abs;
        const auto xi = (i + 1) * h;
        e += abs(u->get_const_values()[i] - correct_u(xi)) / abs(correct_u(xi));
    }
    return e;
}


int main(int argc, char** argv)
{
    const unsigned dp = argc >= 2 ? std::atoi(argv[1]) : 100u;
    auto exec = gko::ReferenceExecutor::create();

    // --- correctness: solve -u'' = 6x, u(0)=0, u(1)=1  (exact u = x^3) ---
    auto correct_u = [](ValueType x) { return x * x * x; };
    auto f = [](ValueType x) { return ValueType{6} * x; };
    auto rhs = vec::create(exec, gko::dim<2>(dp, 1));
    generate_rhs(f, correct_u(0), correct_u(1), rhs.get());
    auto u = vec::create(exec, gko::dim<2>(dp, 1));
    for (std::size_t i = 0; i < dp; ++i) u->get_values()[i] = 0.0;

    gko::solver::Cg<ValueType>::build()
        .with_criteria(gko::stop::Iteration::build().with_max_iters(dp),
                       gko::stop::ResidualNorm<ValueType>::build()
                           .with_reduction_factor(1e-7))
        .on(exec)
        ->generate(Stencil::create(exec, dp, -1, 2, -1))
        ->apply(rhs, u);
    std::printf("%s  solve n=%u   avg rel error %.3e\n", kTag, dp,
                rel_error(dp, u.get(), correct_u) / dp);

    // --- timing: bare apply() loop, size-11 matrix (paper section 7.2) ---
    const std::size_t n = 11;
    const long iters = 40'000'000;
    auto A = Stencil::create(exec, n, -1, 2, -1);
    auto b = vec::create(exec, gko::dim<2>(n, 1));
    auto x = vec::create(exec, gko::dim<2>(n, 1));
    for (std::size_t i = 0; i < n; ++i) b->get_values()[i] = 1.0 + i;
    for (int i = 0; i < 1000; ++i) A->apply(b, x);
    auto t0 = std::chrono::steady_clock::now();
    for (long i = 0; i < iters; ++i) A->apply(b, x);
    auto t1 = std::chrono::steady_clock::now();
    std::printf("%s  %7.2f ns/apply\n", kTag,
                std::chrono::duration<double, std::nano>(t1 - t0).count() /
                    iters);
}
