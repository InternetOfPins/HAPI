/**
 * HAPI HAPIFold benchmark — Mutate<F> and Ref<T,F> pipelines
 *
 * Compile-time probes  (-fsyntax-only -O0 -DTEST_SIZE=N -DTEST_XXX):
 *   TEST_BASELINE    include parse only — no instantiation
 *   TEST_MUTATE      APIOf<MutBase, Mutate<AddK<I>>...>  instantiation
 *   TEST_REF         APIOf<RefBase,  Ref<int,AddK<I>>...> instantiation
 *   TEST_TRANS       APIOf<Identity, Trans<MulK<I>>...>   instantiation (comparison)
 *   TEST_HANA_FOLD   hana::for_each on tuple of N AddK<I> functors
 *
 * Runtime probe  (-O2 -DTEST_FOLD_RUNTIME -DTEST_SIZE=N [-DREPS=K]):
 *   Compares ns/call for:
 *     manual   — hand-written sequential ops (same ops, no abstraction)
 *     mutate   — Mutate<F> chain: pipe.run(v)  (mutates in-place)
 *     ref      — Ref<T,F> chain: pipe.run()    (each Op targets its own int)
 *     trans    — Trans<F> chain: pipe.transform(v)  (functional, for comparison)
 *     hana     — hana::for_each on tuple<AddK<I>...>: fn(v) per element
 */

#ifdef TEST_FOLD_RUNTIME
  #include <chrono>
  #include <iostream>
#endif

#include <hapi/hapi.h>
#include <hapi/run.h>
#include <boost/hana.hpp>
namespace hana = boost::hana;

#ifndef TEST_SIZE
  #define TEST_SIZE 50
#endif
#ifndef REPS
  #define REPS 1000000
#endif

// ── functors — stateless, zero storage ───────────────────────────────────────

template<int K>
struct AddK {
  constexpr void operator()(int& v) const noexcept { v += K; }
};

template<int K>
struct MulK {
  constexpr int operator()(int v) const noexcept { return v * K; }
};

// ── chain generators ──────────────────────────────────────────────────────────

template<typename Seq> struct GenMutate;
template<std::size_t... Is>
struct GenMutate<std::index_sequence<Is...>> {
  using Type = hapi::APIOf<hapi::run::MutBase, hapi::run::Mutate<AddK<(int)Is+1>>...>;
};

template<typename Seq> struct GenRef;
template<std::size_t... Is>
struct GenRef<std::index_sequence<Is...>> {
  using Type = hapi::APIOf<hapi::run::RefBase, hapi::run::Ref<int, AddK<(int)Is+1>>...>;
};

template<typename Seq> struct GenTrans;
template<std::size_t... Is>
struct GenTrans<std::index_sequence<Is...>> {
  using Type = hapi::APIOf<hapi::run::Identity, hapi::run::Trans<MulK<(int)Is+1>>...>;
};

template<typename Seq> struct GenHanaFold;
template<std::size_t... Is>
struct GenHanaFold<std::index_sequence<Is...>> {
  static constexpr auto make() { return hana::make_tuple(AddK<(int)Is+1>{}...); }
  using TupleType = decltype(make());
};

// ── main ─────────────────────────────────────────────────────────────────────

int main() {

#if defined(TEST_BASELINE)
  (void)0;

#elif defined(TEST_MUTATE)
  using Pipe = typename GenMutate<std::make_index_sequence<TEST_SIZE>>::Type;
  { Pipe p; (void)p; }

#elif defined(TEST_REF)
  using Pipe = typename GenRef<std::make_index_sequence<TEST_SIZE>>::Type;
  { Pipe p; (void)p; }

#elif defined(TEST_TRANS)
  using Pipe = typename GenTrans<std::make_index_sequence<TEST_SIZE>>::Type;
  { Pipe p; (void)p; }

#elif defined(TEST_HANA_FOLD)
  {
    constexpr auto fns = GenHanaFold<std::make_index_sequence<TEST_SIZE>>::make();
    int v = 0;
    hana::for_each(fns, [&v](auto fn) { fn(v); });
    (void)v;
  }

#elif defined(TEST_FOLD_RUNTIME)
  using SC = std::chrono::steady_clock;
  using NS = std::chrono::nanoseconds;
  constexpr int N    = TEST_SIZE;
  constexpr int reps = REPS;

  using MutPipe   = typename GenMutate<std::make_index_sequence<N>>::Type;
  using RefPipe   = typename GenRef<std::make_index_sequence<N>>::Type;
  using TransPipe = typename GenTrans<std::make_index_sequence<N>>::Type;

  volatile int g_sink = 0;

  std::cout << "=== HAPIFold runtime  N=" << N << "  reps=" << reps << " ===\n";
  std::cout << "sizeof(MutPipe):   " << sizeof(MutPipe)   << " bytes\n";
  std::cout << "sizeof(RefPipe):   " << sizeof(RefPipe)
            << " bytes  (" << N << " int* = " << N * sizeof(int*) << " bytes expected)\n";
  std::cout << "sizeof(TransPipe): " << sizeof(TransPipe)  << " bytes\n\n";

  // ── manual baseline ────────────────────────────────────────────────────────
  // Same ops as GenMutate: v += 1; v += 2; ...; v += N;
  {
    int warmup = 0;
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
      ((warmup += (int)Is + 1), ...);
    }(std::make_index_sequence<N>{});
    (void)warmup;

    auto t0 = SC::now();
    for (int i = 0; i < reps; ++i) {
      int v = i;
      [&]<std::size_t... Is>(std::index_sequence<Is...>) noexcept {
        ((v += (int)Is + 1), ...);
      }(std::make_index_sequence<N>{});
      g_sink = v;
    }
    long long ns = std::chrono::duration_cast<NS>(SC::now() - t0).count();
    std::cout << "manual   " << (double)ns / reps << " ns/call\n";
  }

  // ── Mutate chain ─────────────────────────────────────────────────────────
  {
    MutPipe pipe;
    int warmup = 0; pipe.run(warmup); (void)warmup;

    auto t0 = SC::now();
    for (int i = 0; i < reps; ++i) {
      int v = i;
      pipe.run(v);
      g_sink = v;
    }
    long long ns = std::chrono::duration_cast<NS>(SC::now() - t0).count();
    std::cout << "mutate   " << (double)ns / reps << " ns/call\n";
  }

  // ── Ref chain ────────────────────────────────────────────────────────────
  // Each Ref<int,AddK<I+1>> targets arr[I]; find<> locates each by its unique type.
  {
    RefPipe pipe;
    int arr[N];

    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
      ((hapi::find<hapi::SameAs<hapi::run::Ref<int,AddK<(int)Is+1>>>>(pipe).target = &arr[Is]), ...);
    }(std::make_index_sequence<N>{});

    // warmup
    [&]<std::size_t... Is>(std::index_sequence<Is...>) { ((arr[Is] = 0), ...); }(std::make_index_sequence<N>{});
    pipe.run();

    auto t0 = SC::now();
    for (int i = 0; i < reps; ++i) {
      [&]<std::size_t... Is>(std::index_sequence<Is...>) noexcept {
        ((arr[Is] = i), ...);
      }(std::make_index_sequence<N>{});
      pipe.run();
      int sum = [&]<std::size_t... Is>(std::index_sequence<Is...>) noexcept {
        return (arr[Is] + ...);
      }(std::make_index_sequence<N>{});
      g_sink = sum;
    }
    long long ns = std::chrono::duration_cast<NS>(SC::now() - t0).count();
    std::cout << "ref      " << (double)ns / reps
              << " ns/call  (includes arr[N] init + sum readback)\n";
  }

  // ── Trans chain (functional reference point) ──────────────────────────────
  {
    TransPipe pipe;
    int warmup = pipe.transform(0); (void)warmup;

    auto t0 = SC::now();
    for (int i = 0; i < reps; ++i)
      g_sink = pipe.transform(i);
    long long ns = std::chrono::duration_cast<NS>(SC::now() - t0).count();
    std::cout << "trans    " << (double)ns / reps << " ns/call\n";
  }

  // ── Hana for_each (same ops as Mutate, Hana's mechanism) ─────────────────
  {
    constexpr auto fns = GenHanaFold<std::make_index_sequence<N>>::make();
    int warmup = 0; hana::for_each(fns, [&warmup](auto fn) { fn(warmup); }); (void)warmup;

    auto t0 = SC::now();
    for (int i = 0; i < reps; ++i) {
      int v = i;
      hana::for_each(fns, [&v](auto fn) { fn(v); });
      g_sink = v;
    }
    long long ns = std::chrono::duration_cast<NS>(SC::now() - t0).count();
    std::cout << "hana     " << (double)ns / reps << " ns/call\n";
  }

  std::cout << "\n(g_sink=" << (int)g_sink << " — prevents DCE)\n";
  return static_cast<int>(g_sink & 0);
#endif

  return 0;
}
