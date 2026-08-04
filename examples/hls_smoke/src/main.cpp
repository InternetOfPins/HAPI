/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief HLS smoke test: the same free/CRTP Chain<> composition as
 *        examples/free, but with an arithmetic side effect instead of
 *        iostream, so the composed API is actually synthesizable by a
 *        high-level-synthesis backend. Verified against PandA-Bambu
 *        2024.10 -- see README.md.
 * @date 2025-04-10
 *
 */
#include <hapi/hapi.h>
#include <type_traits>
using namespace hapi;

/// @brief innermost layer: identity, terminates the chain
struct Item {
  static int api(int o) {return o;}
};

/// @brief a wrapped layer, parameterized by two characters.
/// examples/free prints oc/cc as characters around whatever the next layer
/// below produces; here it adds their ordinal value instead. That is the
/// only change needed to make the composed API synthesizable -- I/O calls
/// have no hardware equivalent, so an HLS backend can't (and shouldn't)
/// accept them inside the function being turned into hardware.
template<char oc, char cc>
struct WrapWith {
  template<typename I>
  struct Part : I {
    static int api(int o) {
      return static_cast<int>(oc) + I::api(o) + static_cast<int>(cc);
    }
  };
};

using Parens   = WrapWith<'(',')'>;
using SqBracks = WrapWith<'[',']'>;
using Bracks   = WrapWith<'{','}'>;
using Bars     = WrapWith<'|','|'>;
using XTag     = WrapWith<'<','>'>;

/// @brief a composition of parts is also a part -- same 5-layer recursive
/// Chain<> collapse as examples/free
using Full = Chain<Parens,SqBracks,Bracks,Bars,XTag>;

using Top = APIOf<Item,Full>;

/// @brief compile-time regression guards -- cheap type-system tripwires
/// that fail the build (no Bambu run needed) if a future HAPI change
/// breaks the properties this example relies on.
/// If a Part<> in the chain ever picks up an accidental virtual method,
/// this catches it before anyone burns an HLS run finding out the hard way
/// (no HLS backend can synthesize a vtable).
static_assert(!std::is_polymorphic_v<Top>,
  "HAPI regression: Chain/APIOf collapse must stay non-polymorphic (no vtable)");
/// If a layer starts carrying state (even one byte), sizeof grows past
/// Item's own footprint -- Empty Base Optimization should hold across
/// any number of stateless layers.
static_assert(sizeof(Top) == sizeof(Item),
  "HAPI regression: layered Chain must not add storage per layer (EBO broken)");
/// Cheap proxy for "still a plain flattened struct" -- something an HLS
/// frontend can map to hardware, not something with nontrivial lifetime.
static_assert(std::is_trivially_destructible_v<Top>,
  "HAPI regression: collapsed chain should stay trivially destructible");

Top testItem;

/// @brief the actual synthesis target -- point an HLS backend's top-function
/// option here (e.g. bambu's --top-fname=wrapSum).
/// `x` is a genuine hardware input port: since it is not a compile-time
/// constant, none of the 5 composed layers can be constant-folded away
/// before scheduling, so the generated RTL reflects the real Chain<>
/// collapse rather than a pre-computed literal.
int wrapSum(int x) {
  return testItem.api(x);
}

#include <iostream>
// host-side sanity check only -- not part of the synthesized design.
// An HLS run targets wrapSum() directly; main() and iostream never enter
// that flow, same as any HLS kernel's host-side test driver.
int main() {
  std::cout << wrapSum(5) << std::endl;
  return 0;
}
