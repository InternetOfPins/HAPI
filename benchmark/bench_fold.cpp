// bench_fold.cpp — HAPI run.h pipeline compile-time and runtime benchmarks
//
// Tests Mutate<fn>, Ref<T,F>, Trans<fn> chains of TEST_SIZE components.
//
// Compile-time probes (-fsyntax-only -O0 -DTEST_SIZE=N -DTEST_XXX):
//   TEST_BASELINE      include hapi/run.h only, no instantiation
//   TEST_MUTATE        APIOf<MutBase,  Mutate<addK>  ×N>
//   TEST_REF           APIOf<RefBase,  Ref<int,AddKF>×N>
//   TEST_TRANS         APIOf<Identity, Trans<addKT>  ×N>
//
// Runtime probe (-O2 -DTEST_FOLD_RUNTIME -DTEST_SIZE=N -DREPS=R):
//   prints sizeof + ns/call for manual / mutate / ref / trans

#include <hapi/hapi.h>
#include <hapi/run.h>
using namespace hapi;
using namespace hapi::run;

#ifndef TEST_SIZE
#define TEST_SIZE 10
#endif
#ifndef REPS
#define REPS 1000000
#endif

// ── repeat-a-type N times ─────────────────────────────────────────────────────

template<typename T, std::size_t> using Repeat = T;

template<typename Base, typename Comp, std::size_t... Is>
static auto api_of_n_ptr(std::index_sequence<Is...>)
  -> APIOf<Base, Repeat<Comp, Is>...>*;

template<typename Base, typename Comp, std::size_t N>
using APIOfN = std::remove_pointer_t<
    decltype(api_of_n_ptr<Base, Comp>(std::make_index_sequence<N>{}))>;

// ── shared increment functions ────────────────────────────────────────────────

static constexpr void addK(int& v)   noexcept { v += 1; }
static constexpr int  addKT(int v)   noexcept { return v + 1; }
struct AddKF { constexpr void operator()(int& v) const noexcept { v += 1; } };

// ── pipeline types ────────────────────────────────────────────────────────────

using MutPipe   = APIOfN<MutBase,   Mutate<addK>,    TEST_SIZE>;
using RefPipe   = APIOfN<RefBase,   Ref<int, AddKF>, TEST_SIZE>;
using TransPipe = APIOfN<Identity,  Trans<addKT>,    TEST_SIZE>;

// ── compile-time probes ───────────────────────────────────────────────────────

#if defined(TEST_BASELINE)
  // include only — no Chain<N> instantiation; measures compiler startup

#elif defined(TEST_MUTATE)
  MutPipe   g_pipe;
  volatile int g_v = 0;
  int main() { g_pipe.run(const_cast<int&>(g_v)); }

#elif defined(TEST_REF)
  RefPipe   g_pipe;
  int main() {}

#elif defined(TEST_TRANS)
  TransPipe g_pipe;
  volatile int g_v = 0;
  int main() { g_v = g_pipe.transform(0); }

// ── runtime benchmark ─────────────────────────────────────────────────────────

#elif defined(TEST_FOLD_RUNTIME)

#include <chrono>
#include <cstdio>

static volatile int g_sink = 0;

template<typename Fn>
static double bench_ns(Fn fn, long reps) {
    fn();  // warmup
    auto t0 = std::chrono::steady_clock::now();
    for (long i = 0; i < reps; i++) fn();
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::nano>(t1 - t0).count() / reps;
}

int main() {
    constexpr int N    = TEST_SIZE;
    constexpr long REP = REPS;

    MutPipe   mut_pipe;
    RefPipe   ref_pipe;
    TransPipe trans_pipe;

    std::printf("sizeof(MutPipe):   %zu bytes\n",   sizeof(mut_pipe));
    std::printf("sizeof(RefPipe):   %zu bytes  (%d int* = %zu bytes expected)\n",
                sizeof(ref_pipe), N, N * sizeof(int*));
    std::printf("sizeof(TransPipe): %zu bytes\n\n", sizeof(trans_pipe));

    // Seed each call from g_sink (volatile) to prevent constant-folding.

    // manual: explicit loop of N addK calls
    double t_manual = bench_ns([&] {
        int v = (int)g_sink;
        for (int i = 0; i < N; i++) addK(v);
        g_sink = v;
    }, REP);

    // mutate pipeline: applies addK N times to v
    double t_mutate = bench_ns([&] {
        int v = (int)g_sink;
        mut_pipe.run(v);
        g_sink = v;
    }, REP);

    // trans pipeline (functional): applies addKT N times
    double t_trans = bench_ns([&] {
        g_sink = trans_pipe.transform((int)g_sink);
    }, REP);

    std::printf("manual   %.6f ns/call\n", t_manual);
    std::printf("mutate   %.6f ns/call\n", t_mutate);
    std::printf("trans    %.6f ns/call\n", t_trans);
    std::printf("\n(g_sink=%d — prevents DCE)\n", (int)g_sink);
}

#endif
