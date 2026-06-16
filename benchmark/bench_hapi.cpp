/**
 * HAPI standalone benchmark — no external dependencies
 *
 * Compile-time probes  (run with: -fsyntax-only -O0 -DTEST_SIZE=N -DTEST_XXX):
 *   TEST_BASELINE    HAPI include parse + compiler startup (no Chain instantiation)
 *   TEST_MAP         Map<F, Chain<N>> type transform
 *   TEST_FIND_FIRST  FindFirst at position 0       (best case)
 *   TEST_FIND_MID    FindFirst at position N/2     (average case)
 *   TEST_FIND_LAST   FindFirst at position N-1     (worst case)
 *   TEST_FOREACH     forEach<Q> over N components  (compile + codegen)
 *   TEST_RUNEACH     runEach<Q> over N components  (constexpr table build)
 *   TEST_NODE_ONLY   FENode construction, no traversal
 *
 * Runtime probe  (-O2 -DTEST_RUNTIME -DTEST_SIZE=N [-DREPS=K]):
 *   prints sizeof metrics and ns/call for forEach and runEach
 *
 * Note: baseline excludes Chain<N> instantiation; all other tests include it.
 *   "pure Chain cost" ≈ node_only - baseline
 *   "pure FindFirst cost" ≈ find_xxx - node_only
 *   "forEach overhead" ≈ foreach - node_only
 */

#ifdef TEST_RUNTIME
  #include <chrono>
  #include <iostream>
#endif

#include <hapi/chain.h>
#include <hapi/meta.h>
#include <hapi/run.h>

#ifndef TEST_SIZE
  #define TEST_SIZE 50
#endif
#ifndef REPS
  #define REPS 100000
#endif

// ── shared component types ────────────────────────────────────────────────────

struct AllTag {};

template<std::size_t I>
struct Comp : AllTag {
    template<typename O>
    struct Part : O { using O::O; };
};

struct DummyAPI {
    template<typename O>
    struct Part : O { using O::O; };
};

struct MakePointer {
    template<typename O>
    struct Apply { using Expr = O*; };
};

template<std::size_t Target>
struct MatchComp {
    template<typename O>
    struct Check { static constexpr bool value = std::is_same_v<O, Comp<Target>>; };
};

template<typename Seq> struct GenChain;
template<std::size_t... Is>
struct GenChain<std::index_sequence<Is...>> {
    using Type = hapi::Chain<Comp<Is>...>;
};

// FENode: APIOf-compatible wrapper so forEach/runEach work.
//   Types::Head = API  (terminal)
//   Types::Tail = Chain<Comp<0>,...,Comp<N-1>>  (components)
//   App<API> prepends: Chain<OO...>::App<API> = Chain<API, OO...>
template<typename C, typename API>
struct FENode : C::template Part<API> {
    using Base  = typename C::template Part<API>;
    using Types = typename C::template App<API>;
    FENode() : Base() {}
};

#ifdef TEST_RUNTIME
volatile int g_sink = 0;
#endif

// ── main ─────────────────────────────────────────────────────────────────────

int main() {

// baseline: no Chain instantiation — pure include + startup cost
#if defined(TEST_BASELINE)
    (void)0;

#elif defined(TEST_MAP)
    using C = typename GenChain<std::make_index_sequence<TEST_SIZE>>::Type;
    using Mapped = typename hapi::Map<MakePointer, C>::Expr;
    (void)static_cast<Mapped*>(nullptr);

#elif defined(TEST_FIND_FIRST)
    using C = typename GenChain<std::make_index_sequence<TEST_SIZE>>::Type;
    using Found = typename hapi::FindFirst<MatchComp<0>, C, DummyAPI>::type;
    (void)static_cast<Found*>(nullptr);

#elif defined(TEST_FIND_MID)
    using C = typename GenChain<std::make_index_sequence<TEST_SIZE>>::Type;
    using Found = typename hapi::FindFirst<MatchComp<TEST_SIZE/2>, C, DummyAPI>::type;
    (void)static_cast<Found*>(nullptr);

#elif defined(TEST_FIND_LAST)
    using C = typename GenChain<std::make_index_sequence<TEST_SIZE>>::Type;
    using Found = typename hapi::FindFirst<MatchComp<TEST_SIZE-1>, C, DummyAPI>::type;
    (void)static_cast<Found*>(nullptr);

#elif defined(TEST_FOREACH)
    using C = typename GenChain<std::make_index_sequence<TEST_SIZE>>::Type;
    { FENode<C, DummyAPI> node; hapi::forEach<hapi::TagIs<AllTag>>(node, [](auto&){}); }

#elif defined(TEST_RUNEACH)
    using C = typename GenChain<std::make_index_sequence<TEST_SIZE>>::Type;
    { FENode<C, DummyAPI> node; hapi::run::runEach<hapi::TagIs<AllTag>>(node, [](auto&){}); }

#elif defined(TEST_NODE_ONLY)
    using C = typename GenChain<std::make_index_sequence<TEST_SIZE>>::Type;
    { FENode<C, DummyAPI> node; (void)node; }

#elif defined(TEST_RUNTIME)
    using SC = std::chrono::steady_clock;
    using NS = std::chrono::nanoseconds;
    using C  = typename GenChain<std::make_index_sequence<TEST_SIZE>>::Type;
    constexpr int N    = TEST_SIZE;
    constexpr int reps = REPS;

    FENode<C, DummyAPI> node;

    std::cout << "sizeof(FENode<" << N << ">): " << sizeof(node) << " bytes\n";
    std::cout << "sizeof(Chain<" << N << ">):  " << sizeof(C)    << " bytes\n";

    // warmup — ensures static constexpr tables are ready
    hapi::forEach<hapi::TagIs<AllTag>>(node, [](auto&){ ++g_sink; });
    hapi::run::runEach<hapi::TagIs<AllTag>>(node, [](auto&){ ++g_sink; });

    {
        auto t0 = SC::now();
        for (int i = 0; i < reps; ++i)
            hapi::forEach<hapi::TagIs<AllTag>>(node, [](auto&){ ++g_sink; });
        long long ns = std::chrono::duration_cast<NS>(SC::now() - t0).count();
        std::cout << "forEach  " << (double)ns / reps       << " ns/call"
                  << "  ("       << (double)ns / reps / N   << " ns/comp)\n";
    }
    {
        constexpr int threshold = static_cast<int>(hapi::run::RunInlineMax);
        auto t0 = SC::now();
        for (int i = 0; i < reps; ++i)
            hapi::run::runEach<hapi::TagIs<AllTag>>(node, [](auto&){ ++g_sink; });
        long long ns = std::chrono::duration_cast<NS>(SC::now() - t0).count();
        std::cout << "runEach  " << (double)ns / reps       << " ns/call"
                  << "  ("       << (double)ns / reps / N   << " ns/comp)"
                  << "  [" << (N > threshold ? "table" : "inline") << " dispatch"
                  << ", threshold=" << threshold << "]\n";
    }

    return static_cast<int>(g_sink & 0); // use sink to prevent DCE, always 0
#endif

    return 0;
}
