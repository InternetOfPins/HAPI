#pragma once
// sugar.h — constexpr factories whose *types* are the net (OneMenu menuDef style); values carry no data
#include "linCell.h"
#include "refid.h"
#include "roll.h"

namespace sugar {
  template<typename Src> struct X {};                       // a source: input slot or another cell
  template<size_t i> constexpr X<snet::Slot<i>> x{};
  template<int v> struct W {};
  template<int v> constexpr W<v> w{};

  namespace detail {
    template<typename C,typename L> struct CellFound;                                   // is the cell type C one of the cells of the Chain L
    template<typename C> struct CellFound<C,hapi::Chain<>> {static constexpr bool value=false;};
    template<typename C,typename... CC> struct CellFound<C,hapi::Chain<C,CC...>> {static constexpr bool value=true;};
    template<typename C,typename H,typename... CC> struct CellFound<C,hapi::Chain<H,CC...>>
      {static constexpr bool value=CellFound<C,hapi::Chain<CC...>>::value;};
  }

  // position of the cell C among the cells L. Not there: a compile error that says so, in the style of RefQ's no-match message
  // (without it the error is an incomplete snet::IndexOf<...>).
  template<typename C,typename L,bool found=detail::CellFound<C,L>::value> struct CellIndex {
    static_assert(snet::detail::never<C>,"sugar::ref: no cell of that type in the net (a reference resolves by the cell's own type: "
                                         "the referenced cell must be one of the arguments of net(...))");
    static constexpr size_t value=0;
  };
  template<typename C,typename L> struct CellIndex<C,L,true> {static constexpr size_t value=snet::IndexOf<C,L>::value;};

  // reference a cell by its own type (identity); resolves to its net index, same j<k rule as Ref<j>.
  // Cells of IDENTICAL type are one cell as far as ref() is concerned: it resolves to the first of them. That is intended, not a
  // bug: identical pure cells compute the same value, so this is deduplication (pinned by sugar_twins.cpp).
  template<typename C> struct RefT {
    template<typename I> SNET_INLINE static constexpr auto get(const I& in) {
      using L=typename I::Net::Cells;
      return snet::Ref<CellIndex<C,L>::value>::get(in);
    }
  };
  template<typename C> constexpr X<RefT<C>> ref(C) {return {};}

  template<typename S,int v> constexpr lin::Term<S,v> operator*(X<S>,W<v>) {return {};}
  template<typename S,int v> constexpr lin::Term<S,v> operator*(W<v>,X<S>) {return {};}

  // The realization is picked here, from the term count: same terms, same result, different code. Unrolled (lin::Cell: one inline
  // multiply-add per term) is faster; rolled (snet::Roll: a table and a loop) is far smaller once there are enough terms.
  //
  // SUGAR_ROLL_AT is the SIZE-POLICY threshold: a policy, not a fact. Measured on AVR on the very cells built here (README, "Realizations"),
  // rolling costs about 3 B per term against about 14-16 B unrolled, at 2.0-2.3x the cycles; the two break even on flash at 5-6 terms
  // (238 B unrolled against 254 B at 5, 282 against 254 at 6). Six is therefore where FLASH breaks even, and above it the factory saves
  // flash and pays cycles: the right default on a 328p, where flash runs out first, but it encodes that objective.
  //   optimise for size   roll from about 6 terms (the default)
  //   optimise for speed  never roll (-DSUGAR_ROLL_AT=1000000), or roll only what the unrolled form would not fit
  // Only AVR was measured. On Cortex-M the unrolled form is one mla per term and the loop is cheap, so the break-even will sit
  // elsewhere; until that is measured, 6 is an AVR number and other targets should set their own.
  #ifndef SUGAR_ROLL_AT
    #define SUGAR_ROLL_AT 6
  #endif
  inline constexpr size_t rollAt=SUGAR_ROLL_AT;

  namespace detail {
    // a term the rolled form can hold: an input slot (Roll reads the raw slot bytes), an index that fits a byte and an int8 weight
    // (Roll stores both as bytes; a wider weight would be truncated silently, so such a cell stays unrolled)
    template<typename T> struct RollTerm {static constexpr bool ok=false;};
    template<typename Acc,typename Prod,size_t i,Acc w> struct RollTerm<lin::TermOf<Acc,Prod,snet::Slot<i>,w>> {
      static constexpr bool ok=(i<256)&&(w>=-128)&&(w<=127);
      using type=snet::T<i,int(w)>;
    };
  }

  template<int b,typename R,typename... TT> constexpr auto cell(R,TT...) {
    if constexpr (sizeof...(TT)>=rollAt && (detail::RollTerm<TT>::ok && ...))
      return hapi::APIOf<lin::API,R,snet::Roll<typename detail::RollTerm<TT>::type...>,lin::Bias<b>>{};
    else
      return lin::Cell<b,R,TT...>{};
  }
  constexpr lin::Sign sign{};

  template<typename... CC> constexpr snet::Net<CC...> net(CC...) {return {};}
}
