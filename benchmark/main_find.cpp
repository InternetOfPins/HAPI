/**
 * @file main_find.cpp
 * @brief Compile-time benchmark: hapi::FindFirst vs Boost.Hana hana::find_if
 *
 * Operation: given a Chain/tuple of N components, find the element
 * matching predicate Q at position: First, Middle, Last.
 *
 * All tests are type-level only — no value instantiated.
 * The result type is forced via static_cast<T*>(nullptr).
 *
 * Compile with:
 *   g++ -std=c++17 -fsyntax-only -I<hapi_include> -DTEST_SIZE=N -DTEST_XXX main_find.cpp
 *
 * TEST_BASELINE         — compiler startup only
 *
 * HAPI (type-level, lazy FindFirst):
 *   TEST_HAPI_FIRST     — match at position 1     (best case)
 *   TEST_HAPI_MIDDLE    — match at position N/2   (average case)
 *   TEST_HAPI_LAST      — match at position N     (worst case)
 *
 * Hana (type-level, hana::find_if on tuple_t):
 *   TEST_HANA_FIRST     — match at position 1
 *   TEST_HANA_MIDDLE    — match at position N/2
 *   TEST_HANA_LAST      — match at position N
 */

#include <type_traits>
#include <boost/hana.hpp>
#include "hapi/meta.h"
#include "hapi/chain.h"

namespace hana = boost::hana;

// -----------------------------------------------------------------------
// Unique tag types for each position
// We need distinct types so the predicate can target a specific one.
// -----------------------------------------------------------------------
template<std::size_t I> struct Tag {};  // Tag<0>, Tag<1>, ..., Tag<N-1>

// -----------------------------------------------------------------------
// Generate Chain<Tag<0>, Tag<1>, ..., Tag<N-1>>
// Each element is a unique type — predicate can target any specific one.
// -----------------------------------------------------------------------
template<typename Seq> struct GenerateChain;
template<std::size_t... Is>
struct GenerateChain<std::index_sequence<Is...>> {
    using Type = hapi::Chain<Tag<Is>...>;
};

// -----------------------------------------------------------------------
// Generate hana::tuple_t<Tag<0>, Tag<1>, ..., Tag<N-1>>
// -----------------------------------------------------------------------
template<typename Seq> struct GenerateHanaTuple;
template<std::size_t... Is>
struct GenerateHanaTuple<std::index_sequence<Is...>> {
    // returns the type of hana::tuple_t<Tag<0>,...,Tag<N-1>> via decltype
    static auto make() -> decltype(hana::tuple_t<Tag<Is>...>);
};

// -----------------------------------------------------------------------
// HAPI predicate: matches Tag<Target> exactly
// -----------------------------------------------------------------------
template<std::size_t Target>
struct MatchTag {
    template<typename O>
    struct Check {
        static constexpr bool value = std::is_same_v<O, Tag<Target>>;
    };
};

// -----------------------------------------------------------------------
// Hana predicate: matches hana::type_c<Tag<Target>>
// -----------------------------------------------------------------------
template<std::size_t Target>
constexpr auto hana_match = hana::trait<[](auto t) {
    return std::is_same_v<typename decltype(t)::type, Tag<Target>>;
}>;
// Note: hana::trait with a lambda requires C++20.
// For C++17 use a struct instead:
template<std::size_t Target>
struct HanaMatchTag {
    template<typename T>
    constexpr auto operator()(T) const {
        return hana::bool_c<std::is_same_v<typename T::type, Tag<Target>>>;
    }
};

// -----------------------------------------------------------------------
// Dummy API for HAPI FindFirst (required third argument).
// FindFirst instantiates:
//   typename O::template Part<typename Chain<OO...>::template Part<API>>
// so API must expose a trivial Part<T> that just inherits T.
// -----------------------------------------------------------------------
struct DummyAPI {
    template<typename T>
    struct Part : T { using T::T; };
};

// Tag<I> also needs a Part<T> for FindFirst to compute MatchedType.
// A trivial pass-through suffices — we only care about the type, not behaviour.
template<std::size_t I>
struct TagComp {
    template<typename T>
    struct Part : T { using T::T; };
    // Expose the tag identity so MatchTag predicate can inspect it
    using TagType = Tag<I>;
};

// Predicate that matches TagComp<Target> by checking its TagType
template<std::size_t Target>
struct MatchTagComp {
    template<typename O>
    struct Check {
        static constexpr bool value = std::is_same_v<O, TagComp<Target>>;
    };
};

// Generator using TagComp instead of raw Tag — each element has Part<>
template<typename Seq> struct GenerateCompChain;
template<std::size_t... Is>
struct GenerateCompChain<std::index_sequence<Is...>> {
    using Type = hapi::Chain<TagComp<Is>...>;
};

// -----------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------
int main() {

#if defined(TEST_BASELINE)
    (void)0;

// ---- HAPI --------------------------------------------------------------

#elif defined(TEST_HAPI_FIRST)
    // Match TagComp<0> — position 1, lazy FindFirst stops immediately
    using C = typename GenerateCompChain<std::make_index_sequence<TEST_SIZE>>::Type;
    using Found = typename hapi::FindFirst<MatchTagComp<0>, C, DummyAPI>::type;
    (void)static_cast<Found*>(nullptr);

#elif defined(TEST_HAPI_MIDDLE)
    // Match TagComp<N/2> — position N/2, lazy FindFirst walks half the chain
    using C = typename GenerateCompChain<std::make_index_sequence<TEST_SIZE>>::Type;
    using Found = typename hapi::FindFirst<MatchTagComp<TEST_SIZE/2>, C, DummyAPI>::type;
    (void)static_cast<Found*>(nullptr);

#elif defined(TEST_HAPI_LAST)
    // Match TagComp<N-1> — position N, lazy FindFirst walks the entire chain
    using C = typename GenerateCompChain<std::make_index_sequence<TEST_SIZE>>::Type;
    using Found = typename hapi::FindFirst<MatchTagComp<TEST_SIZE-1>, C, DummyAPI>::type;
    (void)static_cast<Found*>(nullptr);

// ---- Hana --------------------------------------------------------------

#elif defined(TEST_HANA_FIRST)
    // Match Tag<0> — position 1
    using HT = decltype(GenerateHanaTuple<std::make_index_sequence<TEST_SIZE>>::make());
    using Found = decltype(hana::find_if(
        std::declval<HT>(),
        HanaMatchTag<0>{}
    ));
    (void)static_cast<Found*>(nullptr);

#elif defined(TEST_HANA_MIDDLE)
    // Match Tag<N/2> — position N/2
    using HT = decltype(GenerateHanaTuple<std::make_index_sequence<TEST_SIZE>>::make());
    using Found = decltype(hana::find_if(
        std::declval<HT>(),
        HanaMatchTag<TEST_SIZE/2>{}
    ));
    (void)static_cast<Found*>(nullptr);

#elif defined(TEST_HANA_LAST)
    // Match Tag<N-1> — position N
    using HT = decltype(GenerateHanaTuple<std::make_index_sequence<TEST_SIZE>>::make());
    using Found = decltype(hana::find_if(
        std::declval<HT>(),
        HanaMatchTag<TEST_SIZE-1>{}
    ));
    (void)static_cast<Found*>(nullptr);

#endif

    return 0;
}
