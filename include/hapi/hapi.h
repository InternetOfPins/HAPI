/**
 * @file hapi.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief A powerful modular, zero-overhead, static composition engine for embedded systems and modern C++.
 * */

#pragma once
#include "hapi/rules.h"
#include "hapi/meta.h"

namespace hapi {
  // ====================== APIOf ======================--

  /// @brief Close chain composition with a fallback API, 
  /// collapsing the chain into a single c++ class inheritance
  /// that ultimately derive from the given API
  /// @tparam API : fall-back API
  /// @tparam OO... : the chain components 
  template<typename API, typename... OO>
  struct APIOf : Chain<OO...>::template Part<API> {
    using Base = typename Chain<OO...>::template Part<API>;
    using Base::Base;
    using Types=Chain<API,OO...>;
    static_assert(BuildRules<Chain<>,Chain<OO...>>::rules(), "HAPI: validation failed");
  };

  // Map specialization to traverse directly through an APIOf boundary
  template<typename F, typename API, typename... OO>
  struct Map<F, APIOf<API, OO...>> {
    using Expr = APIOf<API, typename Map<F, OO>::Expr...>;
  };

  // ====================== At<N> ======================--

  /// @brief Indexed homogeneous-array element — zero-overhead tagged storage.
  /// DataDef<At<0,T>, At<1,T>, At<2,T>> is a 3-element array of T with
  /// named positional access via value<K>():
  ///   K==0  → this element's data (base case)
  ///   K>0   → Base::value<K-1>()  (count down through the chain)
  template<std::size_t N, typename T = int>
  struct At {
    template<typename O>
    struct Part : O {
      using Base = O;
      T data{};

      template<typename V, typename... OO>
      constexpr Part(V v, OO&&... oo) noexcept
          : Base{std::forward<OO>(oo)...}, data{static_cast<T>(v)} {}

      template<typename... OO>
      constexpr Part(OO&&... oo) noexcept : Base{std::forward<OO>(oo)...} {}

      T&       get()       noexcept { return data; }
      const T& get() const noexcept { return data; }
      void     set(const T& v) noexcept { data = v; }
      operator T&()       noexcept { return data; }
      operator const T&() const noexcept { return data; }

      template<std::size_t K>
      constexpr T& value() {
        if constexpr (K == 0) return data;
        else                  return Base::template value<K-1>();
      }

      template<std::size_t K>
      constexpr const T& value() const {
        if constexpr (K == 0) return data;
        else                  return Base::template value<K-1>();
      }

      template<typename TT = T>
      constexpr TT& operator[](std::size_t i) {
        if (i == 0) return data;
        return static_cast<Base&>(*this).template operator[]<TT>(i - 1);
      }

      template<typename TT = T>
      constexpr const TT& operator[](std::size_t i) const {
        if (i == 0) return data;
        return static_cast<const Base&>(*this).template operator[]<TT>(i - 1);
      }
    };
  };

  // ====================== Mapped<F> ======================--

  /// @brief Lazy value-transform component — F is a type-level constant (constexpr functor).
  /// static constexpr F fn{} means zero storage regardless of chain size.
  /// value<K>() and operator[](i) apply fn to the result of the underlying walk.
  /// For stateless functors (no captures): sizeof(fn)==0, pure compile-time, EBO-equivalent.
  template<typename F>
  struct Mapped {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;
      static constexpr F fn{};

      template<std::size_t K>
      constexpr auto value() const { return fn(Base::template value<K>()); }

      constexpr auto operator[](std::size_t i) const {
        return fn(static_cast<const Base&>(*this)[i]);
      }
    };
  };

  // ====================== IdxTag<I> ======================--

  /// @brief Positional index tag — zero overhead (EBO), marks component I in a chain.
  /// Place IdxTag<I> before the component it indexes:
  ///   APIOf<API, IdxTag<0>, T0, IdxTag<1>, T1, ...>
  /// Then idx<I>(node) returns a direct reference to Ti.
  template<std::size_t I>
  struct IdxTag {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;
    };
  };

  /// @brief Free function: indexed access into an IdxTag<>-tagged chain.
  /// Returns the value reference for the component tagged IdxTag<I>.
  template<std::size_t I, typename C>
  inline decltype(auto) idx(C& c) { return find<TagIs<IdxTag<I>>>(c); }

  template<std::size_t I, typename C>
  inline decltype(auto) idx(const C& c) { return find<TagIs<IdxTag<I>>>(c); }

}; // namespace hapi