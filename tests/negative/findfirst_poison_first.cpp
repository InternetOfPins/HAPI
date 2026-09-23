// EXPECT-ERROR: FindFirst instantiated a sibling past the match
// Counterpart of the passing case in tests/descent_characterization.cpp: with the poison BEFORE the hit it must
// be reached, proving that test's predicate really would fire if a sibling past a match were ever instantiated.
#include "fx.h"
struct Poison {};
struct Guard {
  template<typename O> struct Apply {
    static_assert(!std::is_same<O,Poison>::value,"FindFirst instantiated a sibling past the match");
    static constexpr bool value = std::is_same<O,A>::value;
  };
};
using R = FindFirst<Guard>::Check<Chain<Poison,A>>;
