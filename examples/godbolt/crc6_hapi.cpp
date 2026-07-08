// Self-contained HAPI + CRC-6 demo for Compiler Explorer.
// HAPI core (chain.h + meta.h + rules.h + APIOf from hapi.h), inlined so this
// compiles standalone with no #include of external HAPI headers -- just
// -std=c++17, no libraries needed. Source: github.com/InternetOfPins/HAPI

#ifdef __AVR__
  #include <stdint.h>
  // AVR toolchains ship no libstdc++ headers at all (no <type_traits>,
  // no <cstdint> std:: mapping), so this trimmed subset -- just
  // void_t/true_type/false_type for HasRules's detection idiom -- is
  // provided by hand instead of included.
  namespace std {
    template<typename _Tp, _Tp __v> struct integral_constant {
      static constexpr _Tp value = __v;
      using value_type = _Tp;
      constexpr operator value_type() const noexcept { return value; }
    };
    template<bool v> using bool_constant = integral_constant<bool, v>;
    using true_type  = bool_constant<true>;
    using false_type = bool_constant<false>;
    template<typename...> using void_t = void;
  }
  using SizeT = unsigned char;
#else
  #include <cstdint>
  #include <type_traits>
  using SizeT = unsigned char;
#endif

namespace hapi {

  // ---- chain.h ----
  struct Nil {};

  template<typename... OO> struct Chain;

  template<>
  struct Chain<> {
    template<typename T>
    struct Part : T { using T::T; };
    using Types = Chain<>;
    static constexpr SizeT size{0};
    template<template<typename...> class W> using Build = W<>;
    template<typename... XX> using App = Chain<XX...>;
    template<typename... XX> using Ins = Chain<XX...>;
    template<template<typename> class M> using Map = Chain<>;
  };

  template<typename O, typename... OO>
  struct Chain<O, OO...> {
    using Types = Chain<O, OO...>;
    using Head  = O;
    using Tail  = Chain<OO...>;
    static constexpr SizeT size{1 + sizeof...(OO)};
    template<template<typename...> class W> using Build = W<O, OO...>;
    template<typename... XX> using App = Chain<XX..., O, OO...>;
    template<typename... XX> using Ins = Chain<O, OO..., XX...>;
    template<template<typename> class M> using Map = Chain<M<O>, M<OO>...>;

    template<typename T>
    struct Part : O::template Part<typename Chain<OO...>::template Part<T>> {
      using Base = typename O::template Part<typename Chain<OO...>::template Part<T>>;
      using Base::Base;
      using Types = Chain<O, OO...>;
    };
  };

  // ---- rules.h (trimmed to what APIOf needs) ----
  template<typename Q, typename O>
  constexpr bool query = false; // no predicate system pulled in for this minimal demo

  template<typename T, typename = void>
  struct HasRules : std::false_type {};
  template<typename T>
  struct HasRules<T, std::void_t<decltype(T::template rules<void,void>())>> : std::true_type {};

  template<typename Current, typename Before, typename After, bool=HasRules<Current>::value>
  struct RuleLayer {
    template<typename O> struct Part : O { using O::rules; };
  };
  template<typename Current, typename Before, typename After>
  struct RuleLayer<Current, Before, After, true> {
    template<typename O>
    struct Part : O {
      static constexpr bool rules() {
        return Current::template rules<Before,After>() && O::rules();
      }
    };
  };
  template<typename Before, typename After>
  struct BuildRules:
    RuleLayer<typename After::Head,Before,typename After::Tail>::template Part<
      hapi::BuildRules<typename Before::template App<typename After::Head>, typename After::Tail>
    >
  {};
  template<typename Before>
  struct BuildRules<Before,Chain<>> {
    static constexpr bool rules() {return true;}
  };

  // ---- hapi.h: APIOf ----
  template<typename API, typename... OO>
  struct APIOf : Chain<OO...>::template Part<API> {
    using Base = typename Chain<OO...>::template Part<API>;
    using Base::Base;
    using Types = Chain<API,OO...>;
    template<typename T>
    using Part = typename Chain<OO...>::template Part<T>;
    static_assert(BuildRules<Chain<>,Chain<OO...>>::rules(), "HAPI: validation failed");
  };

} // namespace hapi

// ---- CRC-6 (x^6+x+1, poly=0x03), left-shift/MSB-first, 2-bit input word ----
// Reference: https://bues.ch/h/crcgen (generated C/Verilog, verified byte-for-byte
// against this HAPI Chain composition for all 64x4 crcIn/data combos).
//
// Every link here is stateless (position/width/poly are all compile-time
// parameters), so transform() is `static constexpr` all the way down -- no
// instance is ever constructed.

struct CRCState { uint8_t crc; uint8_t data; };

template<uint8_t w, uint8_t p>
struct CRC { // terminal / fallback API -- carries Width/Poly for CRCBit to read via O::
  static constexpr const uint8_t width{w};
  static constexpr const uint8_t poly{p};
  static constexpr CRCState transform(CRCState v) { return v; }
};

// Every stage looks at bit 0 of the data it was handed, and shifts data
// right by 1 *before* recursing into O::transform. Execution is
// deepest-first (the last-listed chain entry runs its own step first), so
// shifting on the way down means the deepest stage sees the original data
// pre-shifted by (depth-1) -- i.e. the true MSB ends up at bit 0 -- while
// each shallower stage still holds its own less-shifted copy (v.data, not
// the returned s.data) for its own check.
struct CRCBit {
  template<typename O>
  struct Part : O {
    using O::O;
    static constexpr CRCState transform(CRCState v) {
      CRCState s  = O::transform(CRCState{v.crc, (uint8_t)(v.data >> 1)});
      // fb*Poly (not "if(fb) crc^=Poly") keeps this a single lea-multiply
      // instead of the pricier neg/and branchless idiom -- fb is 0 or 1,
      // Poly==3 here, so fb*Poly is exactly {0, Poly}.
      //
      // No masking here: left-shifting never disturbs bit(Width-1) itself
      // (that bit only ever moves further left, into dead territory), so
      // every stage still reads the correct top bit even with stale high
      // bits above it. Only the final caller-visible result needs masking
      // to (1<<Width)-1 -- see run_crc6 below.
      uint8_t fb  = (uint8_t)(((s.crc >> (O::width - 1)) ^ v.data) & 1u);
      uint8_t crc = (uint8_t)((s.crc << 1) ^ (uint8_t)(fb * O::poly));
      return CRCState{crc, s.data};
    }
  };
};

// Width/Poly are explicit parameters here
// they belong to the algorithm (fixed by the CRC-6 standard, 6 bits wide
// regardless of how many data bits one transform() call processes), not to
// how many links happen to be in the chain.
template<uint8_t Width, uint8_t Poly, typename... Bits>
using CRCDef=hapi::APIOf<CRC<Width,Poly>,Bits...>;

//two bits
using CRC6Pipeline = CRCDef<6, 0x03, CRCBit, CRCBit>;
static_assert((CRC6Pipeline::transform(CRCState{0x15, 0x2}).crc & 0x3F) == 17, "compile-time evaluation check");

//three bits version, just add a component, same optimization level
//your pain goes away here :D
//using CRC6Pipeline = CRCDef<6, 0x03, CRCBit, CRCBit, CRCBit>;
//static_assert((CRC6Pipeline::transform(CRCState{0x15, 0x2}).crc & 0x3F) == 40, "compile-time evaluation check");

// fully static call chain -- no instance needed anywhere
static_assert(sizeof(CRC6Pipeline) == 1, "zero-overhead: chain must still collapse to 1 byte");

// exported for asm inspection on Compiler Explorer
// the & 0x3F here is the only width mask in the whole pipeline -- see the
// no-masking note in CRCBit::transform above.
uint8_t run_crc6(uint8_t crcIn, uint8_t data) {
    return CRC6Pipeline::transform(CRCState{(uint8_t)(crcIn & 0x3F), data}).crc & 0x3F;
}