/**
 * @file compile_tests.cpp
 * @brief HAPI Compile-time validation tests
 * 
 * This file is meant to be compiled (not necessarily run).
 * Heavy use of static_assert + type traits.
 */

#include "../src/hapi.h"
// #include "oneList.h"
// #include "oneData.h"     // when ready

// ====================== Test Features ======================

struct FeatureA {
    template<typename O>
    struct Part : O {
        using HasA = std::true_type;
        static constexpr int id = 1;
    };
};

struct FeatureB {
    template<typename O>
    struct Part : O {
        using HasB = std::true_type;
        static constexpr int id = 2;
    };
};

struct FeatureC {
    template<typename O>
    struct Part : O {
        using HasC = std::true_type;
        static constexpr int id = 3;
    };
};

// Requires / Excludes example
struct NeedsA {
    template<typename O>
    struct Part : O {
        using Requires = TypeList<FeatureA>;
    };
};

struct ConflictsWithB {
    template<typename O>
    struct Part : O {
        using Excludes = TypeList<FeatureB>;
    };
};

// ====================== Actual Tests ======================

static_assert(std::is_same_v<
    decltype(Chain<FeatureA, FeatureB, FeatureC>{}),
    Chain<FeatureA, FeatureB, FeatureC>
>, "Basic Chain failed");

using TestStack = OutDef<
    FeatureA,
    FeatureB,
    NeedsA,
    FeatureC
>;

// Core introspection
static_assert(TestStack::template Has<FeatureA>, "Has<> failed");
static_assert(TestStack::template Has<FeatureB>, "Has<> failed");
static_assert(!TestStack::template Has<FeatureC>, "Has<> should be false here?"); // adjust as needed

// Rules validation
// static_assert(TestStack::template check<NeedsA>(), "Requires rule failed");

// Reordering
using Reordered = TestStack::template Ins<FeatureC, 1>::Type;

// TypeList / OneList integration
auto lst = staticBody(
    item("test1", 42),
    item("test2", "hello")
);

using TL = decltype(lst)::Types;
static_assert(TL::size == 2, "OneList Types mirror failed");
static_assert(TL::template Has<Item<const char*, int>>, "Item type detection failed");

// ====================== Add more test sections here ======================

// TODO:
// - Excludes tests
// - Ins / App / Join reordering tests
// - Virtual facade tests
// - Data components + Watch + NumRange
// - Error message quality (deliberately trigger bad compositions)

// Force compilation
int main() {
    TestStack stack;
    (void)stack;        // silence unused warning
    return 0;
}