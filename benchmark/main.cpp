/**
 * @file main.cpp
 * @brief Compile-time benchmark: hapi vs Boost.Hana
 *
 * Two operations benchmarked:
 *   MAP:  N elements int -> int* (type-level only)
 *   FIND: locate element at position First/Middle/Last in Chain of N
 *
 * All tests type-level only — no value instantiated.
 *
 * Compile with:
 *   g++ -std=c++17 -fsyntax-only -I<hapi_include> -DTEST_SIZE=N -DTEST_XXX main.cpp
 *
 * MAP tests:
 *   TEST_BASELINE       — compiler startup only
 *   TEST_TUPLE_TYPE     — std::tuple<int*...> type alias
 *   TEST_HANA_TYPE      — hana::tuple_t + metafunction, decltype
 *   TEST_HAPI_TYPE      — hapi::Map over Chain<int...>, type alias
 *
 * FIND tests (type-level, lazy):
 *   TEST_HAPI_FIRST     — FindFirst, match at position 1
 *   TEST_HAPI_MIDDLE    — FindFirst, match at position N/2
 *   TEST_HAPI_LAST      — FindFirst, match at position N
 *   TEST_HANA_FIRST     — hana::find_if, match at position 1
 *   TEST_HANA_MIDDLE    — hana::find_if, match at position N/2
 *   TEST_HANA_LAST      — hana::find_if, match at position N
 */

#include <type_traits>
#include <tuple>
#include <boost/hana.hpp>
#include "hapi/chain.h"
#include "hapi/meta.h"

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
// FIND: unique component types with Part<> for HAPI, raw tags for Hana
// -----------------------------------------------------------------------
template<std::size_t I> struct Tag {};  // raw tag for Hana

template<std::size_t I>
struct TagComp {                        // component tag for HAPI
    template<typename T>
    struct Part : T { using T::T; };
};

struct DummyAPI {                       // terminal API for FindFirst
    template<typename T>
    struct Part : T { using T::T; };
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

#endif
    return 0;
}
