/**
 * @file main.cpp
 * @brief Compile-time benchmark: hapi vs Boost.Hana
 *
 * Four operations benchmarked:
 *   MAP:       N elements int -> int* (type-level only)
 *   FIND:      locate element at position First/Middle/Last in flat Chain of N
 *   TREE:      B×B nested Chain (total N=B² elements) — Map and Find
 *   HANA_VAL:  Hana on its own terrain — value-level heterogeneous transform/find
 *
 * Compile with:
 *   g++ -std=c++17 -fsyntax-only -ftemplate-depth=2000 -I<hapi_include> -DTEST_SIZE=N -DTEST_XXX main.cpp
 *   g++ -std=c++17 -fsyntax-only -ftemplate-depth=2000 -I<hapi_include> -DTREE_B=B  -DTEST_XXX main.cpp
 *
 * MAP tests (use TEST_SIZE):
 *   TEST_BASELINE           — compiler startup only
 *   TEST_TUPLE_TYPE         — std::tuple<int*...> type alias
 *   TEST_HANA_TYPE          — hana::tuple_t + metafunction, decltype (type-level)
 *   TEST_HAPI_TYPE          — hapi::Map over Chain<int...>, type alias
 *
 * FIND tests (use TEST_SIZE):
 *   TEST_HAPI_FIRST         — FindFirst, match at position 1
 *   TEST_HAPI_MIDDLE        — FindFirst, match at position N/2
 *   TEST_HAPI_LAST          — FindFirst, match at position N
 *   TEST_HANA_FIRST         — hana::find_if, match at position 1
 *   TEST_HANA_MIDDLE        — hana::find_if, match at position N/2
 *   TEST_HANA_LAST          — hana::find_if, match at position N
 *
 * TREE tests (use TREE_B — total elements = TREE_B²):
 *   TEST_HAPI_TREE_MAP      — Map over B×B nested Chain (native tree walk)
 *   TEST_HAPI_TREE_FIRST    — FindFirst at first leaf  (O(2B) depth)
 *   TEST_HAPI_TREE_LAST     — FindFirst at last leaf   (O(2B) depth)
 *   TEST_HANA_TREE_MAP      — hana::flatten + transform (must destroy tree)
 *   TEST_HANA_TREE_FIND     — hana::flatten + find_if  (must destroy tree)
 *
 * HANA VALUE-LEVEL tests (use TEST_SIZE) — Hana on its own terrain:
 *   TEST_HANA_VAL_MAP       — hana::transform on make_tuple of N values (constexpr)
 *   TEST_HANA_VAL_FIND      — hana::find_if   on make_tuple of N values (constexpr)
 *   TEST_HAPI_VAL_MAP       — hapi::Map value instantiation (HAPI paying value cost)
 */

#include <type_traits>
#include <tuple>
#include <boost/hana.hpp>
#include "hapi/hapi.h"
#include "hapi/run.h"

namespace hana = boost::hana;

// -----------------------------------------------------------------------
// Shared utilities
// -----------------------------------------------------------------------
template<typename T, std::size_t> struct RepeatType { using Type = T; };

// -----------------------------------------------------------------------
// MAP: generators
// -----------------------------------------------------------------------
template<typename T, typename Seq> struct GenerateTupleType;
template<typename T, std::size_t... Is>
struct GenerateTupleType<T, std::index_sequence<Is...>> {
    using Output = std::tuple<typename RepeatType<T*, Is>::Type...>;
};

template<typename T, typename Seq> struct GenerateChainType;
template<typename T, std::size_t... Is>
struct GenerateChainType<T, std::index_sequence<Is...>> {
    using Input = hapi::Chain<typename RepeatType<T, Is>::Type...>;
};

template<typename T, std::size_t... Is>
auto make_hana_type_input(std::index_sequence<Is...>)
    -> decltype(hana::tuple_t<typename RepeatType<T, Is>::Type...>);

struct MakePointer {
    template<typename O>
    struct Apply { using Expr = O*; };
};

// -----------------------------------------------------------------------
// FIND / FOREACH: unique component types with Part<> for HAPI, raw tags for Hana
// -----------------------------------------------------------------------
template<std::size_t I> struct Tag {};

struct AllTag {};  // common base — forEach<TagIs<AllTag>> visits every TagComp<I>

template<std::size_t I>
struct TagComp : AllTag {
    template<typename T>
    struct Part : T { using T::T; };
};

struct DummyAPI {
    template<typename T>
    struct Part : T { using T::T; };
};

// ForEachNode: wraps Chain<OO...> into an APIOf-compatible node for forEach<Q>
// Types::Head = API (terminal), Types::Tail = Chain<OO...> (components)
// App<API> prepends API: Chain<OO...>::App<API> = Chain<API, OO...>
template<typename C, typename API>
struct ForEachNode : C::template Part<API> {
    using Base  = typename C::template Part<API>;
    using Types = typename C::template App<API>;
    ForEachNode() : Base() {}
};

template<std::size_t Target>
struct MatchTagComp {
    template<typename O>
    struct Check {
        static constexpr bool value = std::is_same_v<O, TagComp<Target>>;
    };
};

template<std::size_t Target>
struct HanaMatchTag {
    template<typename T>
    constexpr auto operator()(T) const {
        return hana::bool_c<std::is_same_v<typename T::type, Tag<Target>>>;
    }
};

// Hana value-level predicate — matches hana::int_c<Target>
template<std::size_t Target>
struct HanaValMatch {
    template<typename T>
    constexpr auto operator()(T x) const {
        return hana::bool_c<(T::value == (int)Target)>;
    }
};

template<typename Seq> struct GenerateCompChain;
template<std::size_t... Is>
struct GenerateCompChain<std::index_sequence<Is...>> {
    using Type = hapi::Chain<TagComp<Is>...>;
};

template<typename Seq> struct GenerateHanaTuple;
template<std::size_t... Is>
struct GenerateHanaTuple<std::index_sequence<Is...>> {
    static auto make() -> decltype(hana::tuple_t<Tag<Is>...>);
};

// Hana value-level tuple: hana::make_tuple(hana::int_c<0>, hana::int_c<1>, ...)
// Each element is a distinct compile-time integer constant — Hana's native value domain
template<std::size_t... Is>
constexpr auto make_hana_val_tuple(std::index_sequence<Is...>) {
    return hana::make_tuple(hana::int_c<(int)Is>...);
}

constexpr auto add_one = [](auto x) {
    return hana::int_c<decltype(x)::value + 1>;
};

// HAPI value-level map via Map<Inc, At<N> chain>
// Inc is the same +1 operation as add_one, but as a named type for Map<>
struct Inc { constexpr int operator()(int n) const { return n + 1; } };

struct HapiBenchAPI {
    template<typename O>
    struct Part : O {
        using O::O;
        template<typename T> T&       operator[](std::size_t) noexcept       { __builtin_unreachable(); }
        template<typename T> const T& operator[](std::size_t) const noexcept { __builtin_unreachable(); }
    };
};

template<typename Seq> struct GenAtMapped;
template<std::size_t... Is>
struct GenAtMapped<std::index_sequence<Is...>> {
    using Chain  = hapi::APIOf<HapiBenchAPI, hapi::At<Is>...>;
    using Mapped = typename hapi::Map<Inc, Chain>::Expr;
};

// Trans<Inc> pipeline: N layers of +1 composed bottom-up, zero storage
template<typename Seq> struct GenTransChain;
template<std::size_t... Is>
struct GenTransChain<std::index_sequence<Is...>> {
    template<std::size_t> using TransInc = hapi::run::Trans<Inc>;
    using Type = hapi::APIOf<hapi::run::Identity, TransInc<Is>...>;
};

// std::tuple value-level tuple for comparison
template<std::size_t... Is>
constexpr auto make_std_val_tuple(std::index_sequence<Is...>) {
    return std::make_tuple(std::integral_constant<int, (int)Is>{}...);
}

// -----------------------------------------------------------------------
// TREE: B×B nested structure
// -----------------------------------------------------------------------
template<std::size_t Base, typename Seq> struct GenBranch;
template<std::size_t Base, std::size_t... Is>
struct GenBranch<Base, std::index_sequence<Is...>> {
    using Type = hapi::Chain<TagComp<Base + Is>...>;
};

template<typename Seq> struct GenTree;
template<std::size_t... Bs>
struct GenTree<std::index_sequence<Bs...>> {
    static constexpr std::size_t B = sizeof...(Bs);
    using Type = hapi::Chain<
        typename GenBranch<Bs * B, std::make_index_sequence<B>>::Type...
    >;
};

template<std::size_t Base, typename Seq> struct GenHanaBranch;
template<std::size_t Base, std::size_t... Is>
struct GenHanaBranch<Base, std::index_sequence<Is...>> {
    static auto make() -> decltype(hana::tuple_t<Tag<Base + Is>...>);
};

template<typename Seq> struct GenHanaTree;
template<std::size_t... Bs>
struct GenHanaTree<std::index_sequence<Bs...>> {
    static constexpr std::size_t B = sizeof...(Bs);
    static auto make() -> decltype(hana::make_tuple(
        GenHanaBranch<Bs * B, std::make_index_sequence<B>>::make()...
    ));
};

// -----------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------
int main() {

// ---- MAP tests ---------------------------------------------------------

#if defined(TEST_BASELINE)
    (void)0;

#elif defined(TEST_TUPLE_TYPE)
    using Output = typename GenerateTupleType<int, std::make_index_sequence<TEST_SIZE>>::Output;
    (void)static_cast<Output*>(nullptr);

#elif defined(TEST_HANA_TYPE)
    using HanaInput  = decltype(make_hana_type_input<int>(std::make_index_sequence<TEST_SIZE>{}));
    using HanaOutput = decltype(hana::transform(
        std::declval<HanaInput>(),
        hana::metafunction<std::add_pointer>
    ));
    (void)static_cast<HanaOutput*>(nullptr);

#elif defined(TEST_HAPI_TYPE)
    using H_Input  = typename GenerateChainType<int, std::make_index_sequence<TEST_SIZE>>::Input;
    using H_Output = typename hapi::Map<MakePointer, H_Input>::Expr;
    (void)static_cast<H_Output*>(nullptr);

// ---- FIND tests --------------------------------------------------------

#elif defined(TEST_HAPI_FIRST)
    using C = typename GenerateCompChain<std::make_index_sequence<TEST_SIZE>>::Type;
    using Found = typename hapi::FindFirst<MatchTagComp<0>, C, DummyAPI>::type;
    (void)static_cast<Found*>(nullptr);

#elif defined(TEST_HAPI_MIDDLE)
    using C = typename GenerateCompChain<std::make_index_sequence<TEST_SIZE>>::Type;
    using Found = typename hapi::FindFirst<MatchTagComp<TEST_SIZE/2>, C, DummyAPI>::type;
    (void)static_cast<Found*>(nullptr);

#elif defined(TEST_HAPI_LAST)
    using C = typename GenerateCompChain<std::make_index_sequence<TEST_SIZE>>::Type;
    using Found = typename hapi::FindFirst<MatchTagComp<TEST_SIZE-1>, C, DummyAPI>::type;
    (void)static_cast<Found*>(nullptr);

#elif defined(TEST_HANA_FIRST)
    using HT = decltype(GenerateHanaTuple<std::make_index_sequence<TEST_SIZE>>::make());
    using Found = decltype(hana::find_if(std::declval<HT>(), HanaMatchTag<0>{}));
    (void)static_cast<Found*>(nullptr);

#elif defined(TEST_HANA_MIDDLE)
    using HT = decltype(GenerateHanaTuple<std::make_index_sequence<TEST_SIZE>>::make());
    using Found = decltype(hana::find_if(std::declval<HT>(), HanaMatchTag<TEST_SIZE/2>{}));
    (void)static_cast<Found*>(nullptr);

#elif defined(TEST_HANA_LAST)
    using HT = decltype(GenerateHanaTuple<std::make_index_sequence<TEST_SIZE>>::make());
    using Found = decltype(hana::find_if(std::declval<HT>(), HanaMatchTag<TEST_SIZE-1>{}));
    (void)static_cast<Found*>(nullptr);

// ---- TREE tests --------------------------------------------------------

#elif defined(TEST_HAPI_TREE_MAP)
    using Tree = typename GenTree<std::make_index_sequence<TREE_B>>::Type;
    using TreeMapped = typename hapi::Map<MakePointer, Tree>::Expr;
    (void)static_cast<TreeMapped*>(nullptr);

#elif defined(TEST_HAPI_TREE_FIRST)
    using Tree = typename GenTree<std::make_index_sequence<TREE_B>>::Type;
    using TreeFound = typename hapi::FindFirst<MatchTagComp<0>, Tree, DummyAPI>::type;
    (void)static_cast<TreeFound*>(nullptr);

#elif defined(TEST_HAPI_TREE_LAST)
    using Tree = typename GenTree<std::make_index_sequence<TREE_B>>::Type;
    using TreeFound = typename hapi::FindFirst<MatchTagComp<TREE_B*TREE_B-1>, Tree, DummyAPI>::type;
    (void)static_cast<TreeFound*>(nullptr);

#elif defined(TEST_HANA_TREE_MAP)
    using HanaTree = decltype(GenHanaTree<std::make_index_sequence<TREE_B>>::make());
    using HanaFlat = decltype(hana::flatten(std::declval<HanaTree>()));
    using HanaMapped = decltype(hana::transform(std::declval<HanaFlat>(), hana::metafunction<std::add_pointer>));
    (void)static_cast<HanaMapped*>(nullptr);

#elif defined(TEST_HANA_TREE_FIND)
    using HanaTree = decltype(GenHanaTree<std::make_index_sequence<TREE_B>>::make());
    using HanaFlat = decltype(hana::flatten(std::declval<HanaTree>()));
    using HanaFound = decltype(hana::find_if(std::declval<HanaFlat>(), HanaMatchTag<0>{}));
    (void)static_cast<HanaFound*>(nullptr);

// ---- HANA VALUE-LEVEL tests — Hana on its own terrain -----------------
// These use constexpr values, not type aliases.
// This is what Hana was designed for — heterogeneous value computation.
// HAPI has no equivalent; TEST_HAPI_VAL_MAP shows the cost when forced.

#elif defined(TEST_HANA_VAL_MAP)
    // hana::transform on N compile-time integer values — Hana's native domain
    constexpr auto input  = make_hana_val_tuple(std::make_index_sequence<TEST_SIZE>{});
    constexpr auto output = hana::transform(input, add_one);
    (void)output;

#elif defined(TEST_HAPI_MAPPED)
    // hapi::Map<Inc, At<N> chain> — value-level +1 via Mapped<Inc> wrapper
    // same +1 operation as Hana's add_one, but HAPI stores in memory, Hana encodes in types
    using Node = typename GenAtMapped<std::make_index_sequence<TEST_SIZE>>::Mapped;
    { Node node{}; (void)node; }

#elif defined(TEST_TRANS)
    // hapi::run::Trans<Inc> — N-deep transform pipeline, zero storage, constexpr composition
    using Node = typename GenTransChain<std::make_index_sequence<TEST_SIZE>>::Type;
    { Node node{}; (void)node.transform(0); }

#elif defined(TEST_HANA_VAL_FIND)
    // hana::find_if on N compile-time integer values — Hana's native domain
    constexpr auto input = make_hana_val_tuple(std::make_index_sequence<TEST_SIZE>{});
    constexpr auto found = hana::find_if(input, HanaValMatch<TEST_SIZE/2>{});
    (void)found;

#elif defined(TEST_STD_VAL_MAP)
    // std::apply + std::tuple heterogeneous transform — stdlib baseline for value ops
    constexpr auto std_input  = make_std_val_tuple(std::make_index_sequence<TEST_SIZE>{});
    constexpr auto std_result = std::apply(
        [](auto... xs) { return std::make_tuple(std::integral_constant<int, xs.value+1>{}...); },
        std_input);
    (void)std_result;

#elif defined(TEST_HAPI_FOR_EACH)
    // hapi::forEach<TagIs<AllTag>> — visits every component; O(N) compile-time walk
    using C = typename GenerateCompChain<std::make_index_sequence<TEST_SIZE>>::Type;
    ForEachNode<C, DummyAPI> node;
    hapi::forEach<hapi::TagIs<AllTag>>(node, [](auto&){});


#elif defined(TEST_NODE_ONLY)
    // construct node only — isolates chain construction cost from traversal
    using C = typename GenerateCompChain<std::make_index_sequence<TEST_SIZE>>::Type;
    ForEachNode<C, DummyAPI> node;
    (void)node;

#endif
    return 0;
}
