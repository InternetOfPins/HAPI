/*
 * MIT License
 *
 * Copyright (c) 2025-2026 Internet of Pins
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
// HAPI 0.7.0, single header: include/hapi/*.h inlined by scripts/amalgamate.py. Do not edit;
// regenerate with `python3 scripts/amalgamate.py` (tests/single_header/run.sh checks it is current).
// Headers, in order: hapi/hapi.h, hapi/rules.h, hapi/chain.h, hapi/meta.h, hapi/base.h, hapi/platform/avr/avr_std.h
#pragma once

// ---- begin hapi/hapi.h ----
/**
 * @file hapi.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief A powerful modular, zero-overhead, static composition engine for embedded systems and modern C++.
 * */

// ---- begin hapi/rules.h ----
/**
 * @file rules.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi assembly chain validation
*/

// ---- begin hapi/chain.h ----
/**
 * @file chain.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi chain — mono_block topology.
 *        Chain<O,OO...>::Part<T> = O::Part<Chain<OO...>::Part<T>> collapses
 *        to a single inheritance stack, so a Chain is itself usable as one
 *        component inside another Chain.
*/


// ---- begin hapi/meta.h ----
/**
 * @file meta.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi introspection filter and transformations
*/


// ---- begin hapi/base.h ----
/**
 * @file base.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief HAPI base definitions
*/


// __AVR__ toolchains, and some bare-metal newlib/picolibc cross toolchains
// (e.g. riscv64-unknown-elf-g++ as packaged for rv32e/CH32V003) ship a C
// library but no C++ libstdc++ port -- <cstddef>/<type_traits>/<utility>
// don't exist there at all. Detect via __has_include instead of hardcoding
// __AVR__ as the only such platform.
#if defined(__has_include)
  #define HAPI_HAS_STL_HEADERS __has_include(<cstddef>)
#else
  #define HAPI_HAS_STL_HEADERS 1
#endif

#if defined(__AVR__) || !HAPI_HAS_STL_HEADERS
// ---- begin hapi/platform/avr/avr_std.h ----
/**
 * @file avr_std.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief AVR helper file, patching C++17 for AVRs
 * sourcing from GNU ISO C++ Library with minimal changes.
 */


namespace std {

  #ifdef max
    #undef max
  #endif
  #ifdef min
    #undef min
  #endif
  #ifdef abs
    #undef abs
  #endif

  template<typename O> constexpr O min(O a, O b) { return a < b ? a : b; }
  template<typename O> constexpr O max(O a, O b) { return a > b ? a : b; }
  template<typename X> constexpr X abs(X x) { return x < 0 ? -x : x; }

  using size_t = __SIZE_TYPE__; // was hardcoded `unsigned int` (correct for
                                 // AVR's 16-bit int, wrong for a 32-bit
                                 // freestanding target reusing this shim)

  template<typename T> struct numeric_limits {
    static constexpr T max() noexcept { return ~min(); }
    static constexpr T min() noexcept { return ((T)0) - ((T)1) > 0 ? 0 : (T)(1) << ((sizeof(T) << 3) - 1); }
  };

  // from move.h ------------------------------------------------------------------------
  template<typename _Tp, _Tp __v>
  struct integral_constant {
    static constexpr _Tp                  value = __v;
    typedef _Tp                           value_type;
    typedef integral_constant<_Tp, __v>   type;
    constexpr operator value_type() const noexcept { return value; }
    constexpr value_type operator()() const noexcept { return value; }
  };

  template<typename _Tp, _Tp __v> constexpr _Tp integral_constant<_Tp, __v>::value;

  template<bool v> using bool_constant = integral_constant<bool, v>;

  using true_type  = bool_constant<true>;
  using false_type = bool_constant<false>;

  // Const-volatile modifications
  template<typename _Tp> struct remove_const          { using type = _Tp; };
  template<typename _Tp> struct remove_const<_Tp const> { using type = _Tp; };
  
  template<typename _Tp> struct remove_volatile             { using type = _Tp; };
  template<typename _Tp> struct remove_volatile<_Tp volatile> { using type = _Tp; };
  
  template<typename _Tp> struct remove_cv { using type = _Tp; };
  template<typename _Tp> struct remove_cv<const _Tp> { using type = _Tp; };
  template<typename _Tp> struct remove_cv<volatile _Tp> { using type = _Tp; };
  template<typename _Tp> struct remove_cv<const volatile _Tp> { using type = _Tp; };

  template< class T > using remove_cv_t = typename remove_cv<T>::type;
  template< class T > using remove_const_t = typename remove_const<T>::type;
  template< class T > using remove_volatile_t = typename remove_volatile<T>::type;

  template<typename _Tp> struct add_const    { using type = _Tp const; };
  template<typename _Tp> struct add_volatile { using type = _Tp volatile; };
  template<typename _Tp> struct add_cv       { using type = typename add_const<typename add_volatile<_Tp>::type>::type; };

  template<typename> struct is_lvalue_reference     : false_type {};
  template<typename _Tp> struct is_lvalue_reference<_Tp&> : true_type {};

  template<typename _Tp> struct remove_reference      { using type = _Tp; };
  template<typename _Tp> struct remove_reference<_Tp&>  { using type = _Tp; };
  template<typename _Tp> struct remove_reference<_Tp&&> { using type = _Tp; };

  template< class T > using remove_reference_t = typename remove_reference<T>::type;

  template<typename _Tp>
  [[nodiscard]] constexpr _Tp&& forward(typename remove_reference<_Tp>::type& __t) noexcept {
    return static_cast<_Tp&&>(__t);
  }

  template<typename _Tp>
  [[nodiscard]] constexpr _Tp&& forward(typename remove_reference<_Tp>::type&& __t) noexcept {
    static_assert(!is_lvalue_reference<_Tp>::value, "forward must not convert rvalue to lvalue");
    return static_cast<_Tp&&>(__t);
  }

  #if __cplusplus >= 201703L
  # define _GLIBCXX_NODISCARD [[nodiscard]]
  #else
  # define _GLIBCXX_NODISCARD
  #endif

  template<typename _Tp>
  _GLIBCXX_NODISCARD constexpr typename remove_reference<_Tp>::type&& move(_Tp&& __t) noexcept { 
    return static_cast<typename remove_reference<_Tp>::type&&>(__t); 
  }

  // Conditional Logic
  template<bool, typename, typename> struct conditional;

  template<bool _Cond, typename _Iftrue, typename _Iffalse>
  struct conditional { typedef _Iftrue type; };

  template<typename _Iftrue, typename _Iffalse>
  struct conditional<false, _Iftrue, _Iffalse> { typedef _Iffalse type; };

  template<bool _Cond, typename _Iftrue, typename _Iffalse>
  using conditional_t = typename conditional<_Cond, _Iftrue, _Iffalse>::type;

  template<typename...> using void_t = void;

  template<bool, typename _Tp = void> struct enable_if {};
  template<typename _Tp> struct enable_if<true, _Tp> { typedef _Tp type; };

  template<bool _Cond, typename _Tp = void>
  using enable_if_t = typename enable_if<_Cond, _Tp>::type;

  // Type Inspections
  template<class T> struct is_pointer           : false_type {};
  template<class T> struct is_pointer<T*>       : true_type {};
  template<class T> struct is_pointer<T* const> : true_type {};
  template<class T> struct is_pointer<T* volatile> : true_type {};
  template<class T> struct is_pointer<T* const volatile> : true_type {};

  template< class T > struct remove_pointer                    { typedef T type; };
  template< class T > struct remove_pointer<T*>                { typedef T type; };
  template< class T > struct remove_pointer<T* const>          { typedef T type; };
  template< class T > struct remove_pointer<T* volatile>       { typedef T type; };
  template< class T > struct remove_pointer<T* const volatile> { typedef T type; };
  template< class T > using  remove_pointer_t = typename remove_pointer<T>::type;
  
  namespace detail {
    template <class T> struct type_identity { using type = T; };
    template <class T> auto try_add_lvalue_reference(int) -> type_identity<T&>;
    template <class T> auto try_add_lvalue_reference(...) -> type_identity<T>;
    template <class T> auto try_add_rvalue_reference(int) -> type_identity<T&&>;
    template <class T> auto try_add_rvalue_reference(...) -> type_identity<T>;
  }

  template <class T> struct add_lvalue_reference : decltype(detail::try_add_lvalue_reference<T>(0)) {};
  template <class T> struct add_rvalue_reference : decltype(detail::try_add_rvalue_reference<T>(0)) {};

  template<typename T> typename add_rvalue_reference<T>::type declval() noexcept;

  template<class T, class U> struct is_same : false_type {};
  template<class T> struct is_same<T, T>    : true_type {};

  template< class T, class U > constexpr const bool is_same_v = is_same<T, U>::value;

  template<typename _Tp> struct is_union : public integral_constant<bool, __is_union(_Tp)> {};
  template<typename _Tp> struct is_class : public integral_constant<bool, __is_class(_Tp)> {};

  template<typename _Tp> using __remove_cv_t = typename remove_cv<_Tp>::type;

  template<typename> struct is_const          : public false_type { };
  template<typename _Tp> struct is_const<_Tp const> : public true_type { };

  template<typename _Tp> struct is_function    : public bool_constant<!is_const<const _Tp>::value> { };
  template<typename _Tp> struct is_function<_Tp&>  : public false_type { };
  template<typename _Tp> struct is_function<_Tp&&> : public false_type { };

  template<typename> struct __is_member_function_pointer_helper : public false_type { };
  template<typename _Tp, typename _Cp>
  struct __is_member_function_pointer_helper<_Tp _Cp::*> : public is_function<_Tp>::type { };

  template<typename _Tp>
  struct is_member_function_pointer : public __is_member_function_pointer_helper<__remove_cv_t<_Tp>>::type {};

  template <class, class T, class... Args> struct is_constructible_ : false_type {};
  template <class T, class... Args>
  struct is_constructible_<void_t<decltype(T(declval<Args>()...))>, T, Args...> : true_type {};

  template <class T, class... Args> using is_constructible = is_constructible_<void_t<>, T, Args...>;

  template<int n, typename T>
  [[nodiscard]] const char* bitset(const T val) {
    static char o[n + 1]{0};
    for(int i = 0; i < n; i++) o[n - i - 1] = (val & (1 << i)) ? '1' : '0';
    return o;
  }

  namespace details {
    template<typename B> true_type test_ptr_conv(const volatile B*);
    template<typename> false_type test_ptr_conv(const volatile void*);
    template<typename B, typename D> auto test_is_base_of(int) -> decltype(test_ptr_conv<B>(static_cast<D*>(nullptr)));
    template<typename, typename> auto test_is_base_of(...) -> true_type;
  }
  
  template<typename Base, typename Derived>
  struct is_base_of : integral_constant<bool, is_class<Base>::value && is_class<Derived>::value && decltype(details::test_is_base_of<Base, Derived>(0))::value> {};

  template< class Base, class Derived >
  constexpr bool is_base_of_v = is_base_of<Base, Derived>::value;

  template<class T> struct is_void : is_same<void, typename remove_cv<T>::type> {};

  namespace detail {
    template<class T> auto test_returnable(int) -> decltype(void(static_cast<T(*)()>(nullptr)), true_type{});
    template<class> auto test_returnable(...) -> false_type;
    template<class From, class To> auto test_implicitly_convertible(int) -> decltype(void(declval<void(&)(To)>()(declval<From>())), true_type{});
    template<class, class> auto test_implicitly_convertible(...) -> false_type;
  }
  
  template<class From, class To>
  struct is_convertible : integral_constant<bool,
      (decltype(detail::test_returnable<To>(0))::value && decltype(detail::test_implicitly_convertible<From, To>(0))::value) ||
      (is_void<From>::value && is_void<To>::value)
  > {};

  namespace detail {
    template<class T> struct is_integral_helper : false_type {};
    template<> struct is_integral_helper<bool>               : true_type {};
    template<> struct is_integral_helper<char>                : true_type {};
    template<> struct is_integral_helper<signed char>         : true_type {};
    template<> struct is_integral_helper<unsigned char>       : true_type {};
    template<> struct is_integral_helper<wchar_t>              : true_type {};
    template<> struct is_integral_helper<char16_t>             : true_type {};
    template<> struct is_integral_helper<char32_t>             : true_type {};
    template<> struct is_integral_helper<short>                : true_type {};
    template<> struct is_integral_helper<unsigned short>       : true_type {};
    template<> struct is_integral_helper<int>                  : true_type {};
    template<> struct is_integral_helper<unsigned int>         : true_type {};
    template<> struct is_integral_helper<long>                 : true_type {};
    template<> struct is_integral_helper<unsigned long>        : true_type {};
    template<> struct is_integral_helper<long long>            : true_type {};
    template<> struct is_integral_helper<unsigned long long>   : true_type {};
  }
  template<class T> struct is_integral : detail::is_integral_helper<typename remove_cv<T>::type> {};

  template<class T> constexpr bool is_integral_v = is_integral<T>::value;

  template<class T> struct is_floating_point : false_type {};
  template<> struct is_floating_point<float>       : true_type {};
  template<> struct is_floating_point<double>      : true_type {};
  template<> struct is_floating_point<long double> : true_type {};

  template<class T> struct is_array : false_type {};
  template<class T> struct is_array<T[]> : true_type {};
  template<class T, size_t N> struct is_array<T[N]> : true_type {};

  template< class T > constexpr bool is_array_v = is_array<T>::value;

  namespace detail {
    template<class T> auto try_add_pointer(int) -> type_identity<typename remove_reference<T>::type*>;
    template<class T> auto try_add_pointer(...) -> type_identity<T>;
  }
  
  template<class T> struct add_pointer : decltype(detail::try_add_pointer<T>(0)) {};

  template<class T> struct remove_extent { using type = T; };
  template<class T> struct remove_extent<T[]> { using type = T; };
  template<class T, size_t N> struct remove_extent<T[N]> { using type = T; };

  template< class T > using remove_extent_t = typename remove_extent<T>::type;

  template<class T>
  struct decay {
  private:
    typedef typename remove_reference<T>::type U;
  public:
    typedef typename conditional< 
      is_array<U>::value,
      typename add_pointer<typename remove_extent<U>::type>::type,
      typename conditional< 
        is_function<U>::value,
        typename add_pointer<U>::type,
        typename remove_cv<U>::type
      >::type
    >::type type;
  };

  template< class T > using decay_t = typename decay<T>::type;

} // namespace std
// ---- end hapi/platform/avr/avr_std.h ----
  namespace hapi { using SizeT=__SIZE_TYPE__; }
#else
  #include <cstddef>
  #include <type_traits>
  #include <utility>
  namespace hapi { using SizeT=size_t; }
#endif

#ifdef HAPI_DEBUG
  #include <iostream>
  using std::cout;
  using std::endl;
  namespace hapi{};
#endif

// ---- end hapi/base.h ----

//the happy API
namespace hapi {

  template<typename... OO> struct Chain;

  template<typename O> struct Left  { using Type=O; };
  template<typename O> struct Right { using Type=O; };

  template<int V> struct Tag {
    static constexpr int value{V};
    template<typename O> struct Part : O { using O::O; };
  };

  template<typename T> struct Succ;
  template<int V> struct Succ<Tag<V>> { using Type = Tag<V+1>; };

  // ── Expand<O>: what a container holds, taught once per container ───────────────
  // The structural fact behind every walk that needs to open a container: an
  // ordered Chain<...> of children. Primary = leaf: declared but never defined, so
  // asking about a leaf instantiates nothing (the walks probe every element). Opt-in per
  // EXACT type, deliberately never keyed on ::Types: ::Types is a convention on
  // many unrelated types, and a generic ::Types splice was tried and reverted
  // (HAPI commit 7c5e779) for breaking whole-object Filter<FromTypes<..>> in
  // OneMenu. A derived type (e.g. an ItemDef deriving from APIOf) needs its own
  // entry, one line forwarding to its base's:
  //   template<typename... OO> struct Expand<D<OO...>> : Expand<B<OO...>> {};
  // and, like any specialization, it must be declared before Expand is first
  // used on that type.
  //
  // The four policy bits say WHICH walks descend, because they genuinely differ
  // and unifying the structure must not silently unify the policy:
  //   queried    Any/Exists/query/Requires/Excludes look inside (ops with is_query)
  //   selected   every other Traverse op (Filter/Map/Partition/...) looks inside;
  //              false = the element is taken whole
  //   validates  BuildRules/NoCollision splice it into the rule walk
  //   searched   FindFirst opens it, after testing the node itself
  // Every bit defaults to false so an entry lists only what it enables.
  template<typename Kids, bool Queried=false, bool Selected=false, bool Validates=false, bool Searched=false>
  struct Expansion {
    using Children = Kids;
    static constexpr bool queried   = Queried;
    static constexpr bool selected  = Selected;
    static constexpr bool validates = Validates;
    static constexpr bool searched  = Searched;
  };

  template<typename O> struct Expand;   // leaf: declared, never defined, so probing a leaf instantiates nothing

  // a plain Chain is transparent: every walk goes through it
  template<typename... OO>
  struct Expand<Chain<OO...>> : Expansion<Chain<OO...>,true,true,true,true> {};

  template<typename O, typename = void> struct IsContainer : std::false_type {};
  template<typename O>
  struct IsContainer<O, std::void_t<typename Expand<O>::Children>> : std::true_type {};

  // per-walk lookups: false for a leaf, else the container's own bit
  // one SFINAE probe of Expand<O> each (no IsContainer layer): for a leaf the probe instantiates no class
  template<typename O, typename = void> struct Validates : std::false_type {};
  template<typename O> struct Validates<O, std::void_t<typename Expand<O>::Children>> : std::bool_constant<Expand<O>::validates> {};
  template<typename O, typename = void> struct Searches : std::false_type {};
  template<typename O> struct Searches<O, std::void_t<typename Expand<O>::Children>> : std::bool_constant<Expand<O>::searched> {};

  // is the first element of a list one the rule walks splice in place? (false for an empty list)
  template<typename L, typename = void> struct HeadValidates : std::false_type {};
  template<typename H, typename... TT>
  struct HeadValidates<Chain<H,TT...>, std::void_t<typename Expand<H>::Children>> : std::bool_constant<Expand<H>::validates> {};

  // Traverse ops: a query op (is_query, i.e. Any) uses `queried`, every other op uses `selected`
  template<typename Op, typename = void> struct IsQueryOp : std::false_type {};
  template<typename Op> struct IsQueryOp<Op, std::void_t<decltype(Op::is_query)>> : std::bool_constant<Op::is_query> {};
  template<typename Op, typename O, typename = void> struct Opens : std::false_type {};
  template<typename Op, typename O>
  struct Opens<Op,O, std::void_t<typename Expand<O>::Children>> : std::bool_constant<IsQueryOp<Op>::value ? Expand<O>::queried : Expand<O>::selected> {};

  // ── Traverse: the ONLY container extension point ───────────────────────────────--

  template<typename Op, typename Input> struct Traverse;

  template<typename Op, typename Input>
  using Eval = typename Traverse<Op, Input>::Beta;

  // a leaf, or a container this Op opens (Expand<Input>::queried/selected): apply Op per child, then combine
  template<typename Op, typename Kids> struct TraverseKids;
  template<typename Op, typename... OO>
  struct TraverseKids<Op, Chain<OO...>> {
    using Beta = typename Op::template ApplyPack<typename Traverse<Op, OO>::Beta...>;
  };
  // the two branches are alias templates inside a class per bool, not a class per (Op,Input): only the branch taken
  // is substituted, and a leaf costs no extra class instantiation beyond Traverse and Opens themselves
  template<bool Open> struct TraverseBeta;
  template<> struct TraverseBeta<false> { template<typename Op, typename Input> using Of = typename Op::template Apply<Input>; };
  template<> struct TraverseBeta<true>  { template<typename Op, typename Input> using Of = typename TraverseKids<Op, typename Expand<Input>::Children>::Beta; };

  // the extension point is unchanged: a type may still specialize Traverse<Op,X<...>> by hand,
  // and that specialization wins over this primary.
  template<typename Op, typename Input>
  struct Traverse { using Beta = typename TraverseBeta<Opens<Op,Input>::value>::template Of<Op,Input>; };

  // fast path for the container every walk descends (= Expand<Chain>: all bits on), no lookups
  template<typename Op, typename... OO>
  struct Traverse<Op, Chain<OO...>> {
    using Beta = typename Op::template ApplyPack<typename Traverse<Op, OO>::Beta...>;
  };

  // ── Predicates ───────────────────────────────────────────────────────────────--

  template<typename Q>
  struct SameAs {
    template<typename O> using Check    = typename Traverse<SameAs<Q>,O>::Beta;
    template<typename O> using Apply    = std::is_same<Q,O>;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  template<typename Tag>
  struct TagIs {
    template<typename O> using Check    = typename Traverse<TagIs<Tag>,O>::Beta;
    template<typename O> using Apply    = std::is_base_of<Tag,O>;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  template<template<typename...> class Wrapper>
  struct IsInstanceOf {
    template<typename O> using Check = typename Traverse<IsInstanceOf<Wrapper>,O>::Beta;
    template<typename O> struct Apply : std::false_type {};
    template<typename... OO> struct Apply<Wrapper<OO...>> : std::true_type {};
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  template<typename Q,template<typename> class L=Left,template<typename> class R=Right>
  struct Partition {
    template<typename O> using Check    = typename Traverse<Partition<Q>,O>::Beta;
    template<typename O> using Apply = std::conditional_t<Q::template Apply<O>::value, L<O>, R<O>>;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  // ── Predicate combinators ───────────────────────────────────────────────────────--
  // compose at leaf level (Apply); Check/ApplyPack just let these stand in
  // anywhere a predicate is expected — same shape as SameAs.

  template<typename Q>
  struct Not {
    template<typename O> using Check    = typename Traverse<Not<Q>,O>::Beta;
    template<typename O> using Apply    = std::bool_constant<!Q::template Apply<O>::value>;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  template<typename A, typename B>
  struct And {
    template<typename O> using Check    = typename Traverse<And<A,B>,O>::Beta;
    template<typename O> using Apply    = std::bool_constant<
      A::template Apply<O>::value && B::template Apply<O>::value>;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  template<typename A, typename B>
  struct Or {
    template<typename O> using Check    = typename Traverse<Or<A,B>,O>::Beta;
    template<typename O> using Apply    = std::bool_constant<
      A::template Apply<O>::value || B::template Apply<O>::value>;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  // ── Map ─────────────────────────────────────────────────────────────────────────--
  // leaf-level transform F<O>::Type; ApplyPack just rebuilds the Chain shape,
  // so nested Chains map structurally without Map having to know about Chain itself.

  template<template<typename> class F>
  struct Map {
    template<typename O> using Check    = typename Traverse<Map<F>,O>::Beta;
    template<typename O> using Apply    = typename F<O>::Type;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  template<template<typename> class F, typename Input>
  using Transform = Eval<Map<F>, Input>;

  // ── Fold: Any ──────────────────────────────────────────────────────────────────--
  // Recursive, not a fold expression: MSVC rejects a unary fold over a
  // dependent pack member (e.g. (OO::value || ...)). Not std::disjunction --
  // AVR's <type_traits> shim (avr_std.h) only provides bool_constant.

  template<typename... OO> struct OrPack;
  template<> struct OrPack<> : std::false_type {};
  template<typename O, typename... OO>
  struct OrPack<O, OO...> : std::bool_constant<O::value || OrPack<OO...>::value> {};

  template<typename Q>
  struct Any {
    static constexpr bool is_query = true;   // opens containers by their `queried` bit
    template<typename O> using Check = typename Traverse<Any<Q>, O>::Beta;
    template<typename O> using Apply = typename Q::template Apply<O>;
    template<typename... OO> using ApplyPack = OrPack<OO...>;
  };

  // ── Filter ─────────────────────────────────────────────────────────────────────--
  // right-fold: splice a pack of fragment-Chains into one, preserving order

  template<typename... Fragments> struct ConcatChains;
  template<> struct ConcatChains<> { using Type = Chain<>; };
  template<typename... OO, typename... Rest>
  struct ConcatChains<Chain<OO...>, Rest...> {
    template<typename... RR> struct Splice;
    template<typename... RR> struct Splice<Chain<RR...>> { using Type = Chain<OO...,RR...>; };
    using Type = typename Splice<typename ConcatChains<Rest...>::Type>::Type;
  };

  template<typename Q>
  struct Filter {
    template<typename O> using Check = typename Traverse<Filter<Q>, O>::Beta;
    template<typename O> using Apply = std::conditional_t<Q::template Check<O>::value,Chain<O>,Chain<>>;
    template<typename... OO> using ApplyPack = typename ConcatChains<OO...>::Type;
  };

  // ── FindFirst ──────────────────────────────────────────────────────────────────--
  // own head/tail walk (not routed through Traverse): stops at the first match,
  // never instantiates a sibling past it. No Just/Nothing — querying a chain with
  // no match is a compile error (dead end has no ::Result member to access).

  template<typename Q, typename Input> struct FindFirst_;

  // detect whether a (possibly nested) search already produced a Result
  template<typename T, typename = void> struct HasResult : std::false_type {};
  template<typename T> struct HasResult<T, std::void_t<typename T::Result>> : std::true_type {};

  // leaf: Result present only if the predicate accepts it
  template<typename Q, typename Input, bool=Q::template Apply<Input>::value>
  struct FindFirstLeaf {}; // miss: no Result member
  template<typename Q, typename Input>
  struct FindFirstLeaf<Q,Input,true> { using Result = Input; };

  // A container that opts in (Expand<Input>::searched) is tested as a whole FIRST and only then opened:
  // Q may want the container itself (agnosticism's refid matches a whole APIOf cell through FromTypes).
  // Chain is not handled here, it is the walk itself (specializations below) and is never matched as an element.
  template<typename Q, typename Input, bool Hit>
  struct FindFirstOpen : FindFirst_<Q, typename Expand<Input>::Children> {};   // miss: open it
  template<typename Q, typename Input>
  struct FindFirstOpen<Q,Input,true> : FindFirstLeaf<Q,Input> {};              // hit: the node itself

  // leaf, or a container that is not searched: just the leaf test; a searched container: node first, then open it
  template<bool Search> struct FindFirstWith;
  template<> struct FindFirstWith<false> { template<typename Q, typename Input> using Of = FindFirstLeaf<Q,Input>; };
  template<> struct FindFirstWith<true>  { template<typename Q, typename Input> using Of = FindFirstOpen<Q,Input,HasResult<FindFirstLeaf<Q,Input>>::value>; };

  template<typename Q, typename Input>
  struct FindFirst_ : FindFirstWith<Searches<Input>::value>::template Of<Q,Input> {};

  // chain: dispatch on whether Head's search already has a Result; each branch
  // only names the chain element it actually needs, so the miss-branch's lone
  // reference to Chain<TT...> is the only place that ever instantiates the tail
  template<typename Q, typename HeadSearch, typename Tail, bool=HasResult<HeadSearch>::value>
  struct FindFirstChain : FindFirst_<Q,Tail> {};               // miss: try tail
  template<typename Q, typename HeadSearch, typename Tail>
  struct FindFirstChain<Q,HeadSearch,Tail,true> : HeadSearch {}; // hit: stop here

  template<typename Q, typename H, typename... TT>
  struct FindFirst_<Q, Chain<H,TT...>> : FindFirstChain<Q, FindFirst_<Q,H>, Chain<TT...>> {};

  template<typename Q>
  struct FindFirst_<Q, Chain<>> {}; // dead end: no Result

  template<typename Q>
  struct FindFirst {
    template<typename Input> using Check = typename FindFirst_<Q, Input>::Result;
  };

  // ── Soft-fail variants ─────────────────────────────────────────────────────────--

  /// @brief presence-only check: never fails to compile, just answers true/false.
  template<typename Q, typename Input>
  using Exists = typename Any<Q>::template Check<Input>;

  // find-or-default: same head/tail walk as FindFirst, but a miss yields
  // Default instead of a compile error. Still short-circuits via FindFirstChain.
  template<typename Q, typename Default, typename Input, bool=HasResult<FindFirst_<Q,Input>>::value>
  struct FindFirstOrLeaf { using Result = Default; }; // miss: fall back
  template<typename Q, typename Default, typename Input>
  struct FindFirstOrLeaf<Q,Default,Input,true> { using Result = typename FindFirst_<Q,Input>::Result; };

  template<typename Q, typename Default>
  struct FindFirstOr {
    template<typename Input> using Check = typename FindFirstOrLeaf<Q,Default,Input>::Result;
  };

  /// @brief drill into O::Types and apply Q to it; only true if O has ::Types
  template<typename Q>
  struct FromTypes {
    template<typename O, typename = void>
    struct Apply : std::false_type {};  // no ::Types member

    template<typename O>
    struct Apply<O, std::void_t<typename O::Types>>
      : std::bool_constant<Exists<Q, typename O::Types>::value> {};

    template<typename O> using Check = typename Traverse<FromTypes<Q>, O>::Beta;
    template<typename... OO> using ApplyPack = Chain<OO...>;
  };

  /// @brief compatibility shim for the rules system (Requires/Excludes/APIOf):
  /// boolean presence of predicate Q anywhere in O, as a constexpr value (not a type).
  template<typename Q, typename O>
  constexpr bool query = Exists<Q,O>::value;

  // ── At<N> ──────────────────────────────────────────────────────────────────────--
  // IsConst is threaded separately (not read off O at each step) because
  // O::Base is looked up on the possibly-const O but a nested typedef name
  // never itself carries constness -- without this, at() on a const object
  // silently loses const one level up and static_cast then fails to compile
  // ("casting away const") instead of just working.

  template<std::size_t idx,typename O,bool IsConst=std::is_const<O>::value>
  struct At {
    using Type=typename hapi::At<idx-1,typename std::remove_const_t<O>::Base,IsConst>::Type;
  };
  template<typename O,bool IsConst>
  struct At<0,O,IsConst> { using Type=std::conditional_t<IsConst,const O,O>; };

  // Three overloads instead of the old auto-NTTP form (template<auto ref>):
  // a class-type object bound by value as a non-type template parameter is
  // C++20-only (P1907) and this project stays on C++17, so the NTTP form
  // could never actually be called. Ordinary function parameters have no
  // such restriction and additionally work on locals/temporaries, which a
  // linkage-requiring NTTP never could either way.
  template<std::size_t idx,typename O>
  [[nodiscard]] constexpr auto& at(O& obj) {return static_cast<typename At<idx,O>::Type&>(obj);}

  template<std::size_t idx,typename O>
  [[nodiscard]] constexpr auto& at(O* obj) {return at<idx>(*obj);}

  /// @brief for a temporary: bind the result to a named reference first
  /// (e.g. `auto&& tmp=T{}; at<idx>(tmp);`) -- otherwise the returned
  /// reference dangles past the end of the full expression, same as any
  /// function returning a reference into an rvalue argument.
  template<std::size_t idx,typename O>
  [[nodiscard]] constexpr auto&& at(O&& obj) {return static_cast<typename At<idx,O>::Type&&>(obj);}

  // ── Runtime query functions ────────────────────────────────────────────────────--

  /// @brief find first match of Q in C's ::Types: verify Q matches, return full object ref.
  template<typename Q, typename C>
  [[nodiscard]] decltype(auto) find(C& c) {
    using Types = typename C::Types;
    static_assert(HasResult<FindFirst_<Q, Types>>::value, "find: predicate Q not found in C::Types");
    return c;
  }

};
// ---- end hapi/meta.h ----

namespace hapi {
  /// @brief sentinel empty type
  struct Nil {};

  // ====================== CHAIN ======================--

  template<typename... OO> struct Chain;

  // ── Drop<n>: the chain suffix after skipping its first n elements ──────────
  // Haskell's `drop n xs`; Nth is then just Drop<n>::Head. Kept total: dropping
  // past the end gives Chain<> rather than an error (an out-of-range Head
  // access still fails naturally, since Chain<> has no Head). Recursion is by
  // partial specialization, not std::conditional_t<n==0,...,...>: conditional_t
  // requires both branches to already be valid types, so it would eagerly
  // instantiate the n-1 branch even at n==0, underflowing the unsigned SizeT
  // and never terminating. Checked with static_asserts on host g++ and on
  // avr-gcc 7.3 (no STL).
  template<SizeT n,typename L> struct DropOf {using Type=typename DropOf<n-1,typename L::Tail>::Type;};
  template<typename L>         struct DropOf<0,L>       {using Type=L;};
  template<SizeT n>            struct DropOf<n,Chain<>> {using Type=Chain<>;};
  template<>                   struct DropOf<0,Chain<>> {using Type=Chain<>;};  // disambiguates the two specializations above at n=0,L=Chain<>

  /// Empty chain
  template<>
  struct Chain<> {
    template<typename T>
    using Part = T;  // anchor: no more components, collapse to T
    using Types = Chain<>;
    static constexpr SizeT size{0};
    template<template<typename...> class W> using Build = W<>;
    template<typename... XX> using App = Chain<XX...>;
    template<typename... XX> using Ins = Chain<XX...>;
    template<template<typename> class M> using Map = Chain<>;
    template<SizeT n> using Drop = Chain<>;
  };

  // list of types
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
    template<SizeT n> using Drop = typename DropOf<n,Chain<O,OO...>>::Type;

    // A bare Chain<> used directly (no APIOf) has no validation hook of its
    // own -- name collisions between siblings (same method, different
    // signature -- one silently hides the other) go unchecked unless you
    // opt in yourself: see rules.h's NoCollision, usable standalone in a
    // static_assert at your own composition site.
    template<typename T>
    struct Part : O::template Part<typename Chain<OO...>::template Part<T>> {
      using Base = typename O::template Part<typename Chain<OO...>::template Part<T>>;
      using Base::Base;
      using Types = Chain<O, OO...>;
    };
  };

  /// @brief provide circular reference to the whole chain if needed
  template<typename O>
  struct CRTP {
    using Obj=O;
    [[nodiscard]] O& obj() {return static_cast<O&>(*this);}
    [[nodiscard]] const O& obj() const {return static_cast<const O&>(*this);}
    [[nodiscard]] O* operator->() {return static_cast<O*>(this);}
    [[nodiscard]] const O* operator->() const {return static_cast<const O*>(this);}
  };

}; // namespace hapi
// ---- end hapi/chain.h ----
// #include "hapi/meta.h" -- inlined above

namespace hapi {
  /// @brief true if predicate X matches at least one element in any of Chains.
  /// Pass After only for directional checks; pass Before,After for full-chain checks.
  template<typename X, typename... Chains>
  inline constexpr bool Requires = []() {
    static_assert(sizeof...(Chains) > 0, "Requires<X>: no chain specified — pass After, or Before+After for full-chain check");
    return (query<X, Chains> || ...);
  }();

  /// @brief true if predicate X matches no element in any of Chains.
  /// Pass After only for directional checks; pass Before,After for full-chain checks.
  template<typename X, typename... Chains>
  inline constexpr bool Excludes = []() {
    static_assert(sizeof...(Chains) > 0, "Excludes<X>: no chain specified — pass After, or Before+After for full-chain check");
    return (!query<X, Chains> && ...);
  }();
  // ====================== RULES DETECTION ======================--

  template<typename T, typename = void>
  struct HasRules : std::false_type {};

  template<typename T>
  struct HasRules<T, std::void_t<decltype(T::template rules<void,void>())>> 
    : std::true_type {};

  // ====================== BEFORE / AFTER WALK ======================--

  // default case, target has no rules, call next valid rules, 
  // in practice only the last level match this case (if not having rules itself)
  template<typename Current, typename Before, typename After, bool=HasRules<Current>::value>
  struct RuleLayer {
    template<typename O> struct Part : O {using O::rules;};
  };

  /// @brief rules fold/collapse utility, compose all rules into a single object.
  template<typename Current, typename Before, typename After>
  struct RuleLayer<Current, Before, After, true> {
    template<typename O>
    struct Part : O {
      [[nodiscard]] static constexpr bool rules() {
        return Current::template rules<Before,After>() && O::rules();
      }
    };
  };

  /// @brief starts the rules folding process, walking the list of types to provide
  /// correct before/after elements to each target element in the chain. A head that
  /// is a container with `validates` (Chain, APIOf, ...) is first replaced by its
  /// children in place, so ITS elements' rules see the right Before/After context.
  template<typename Before, typename After, bool = HeadValidates<After>::value>
  struct BuildRules;

  template<typename Before, typename After>
  struct BuildRules<Before, After, false>:
    RuleLayer<typename After::Head,Before,typename After::Tail>::template Part<
      hapi::BuildRules<typename Before::template App<typename After::Head>, typename After::Tail>
    >
  {};

  //rules fold termination
  template<typename Before>
  struct BuildRules<Before,Chain<>,false> {
    [[nodiscard]] static constexpr bool rules() {return true;}
  };

  /// @brief does a container that is spliced into the rule walk carry a rules() of its own? Chain and APIOf never do,
  /// and must not be probed: HasRules needs a complete type, and for a nested APIOf that would instantiate the whole
  /// composed class (and fire its own validation as a hard error instead of letting the walk report false). Any other
  /// container is asked (e.g. a printer wrapper that has a rule of its own).
  template<typename O> struct HasOwnRules : HasRules<O> {};
  template<typename... OO> struct HasOwnRules<Chain<OO...>> : std::false_type {};

  // The container's OWN rules() (if it has any) still runs, with the same Before/After it would see if placed directly
  // (its later siblings, not its own children): only then is it replaced by its children. Dropping it would silently
  // switch off a rule that is live when the same component is placed directly.
  template<typename Before, typename After>
  struct BuildRules<Before, After, true>
    : RuleLayer<typename After::Head, Before, typename After::Tail, HasOwnRules<typename After::Head>::value>::template Part<
        BuildRules<Before, typename ConcatChains<typename Expand<typename After::Head>::Children,
                                                 typename After::Tail>::Type>
      > {};

  // ====================== MEMBER COLLISION DETECTION ======================--
  // Ordinary C++ name lookup silently hides one same-named method behind
  // another when two Chain<> siblings declare it with different signatures
  // (found for real in .RnD/focCompose: Sensor's void init() vs. Driver's
  // int init(), folded into one Chain -- only the driver's stayed reachable,
  // no error, no warning). C++17 has no reflection over member names, so
  // this can't be fully automatic -- decltype(&T::name) needs `name`
  // spelled literally by whoever already knows it matters (the API/contract
  // author, same "mirror the real names" convention APIOf consumers already
  // follow). Two accepted, documented limitations, not silently swallowed:
  // (1) an overloaded name on the probed type makes &T::name ill-formed, so
  // Has<T> reports false -- a silent miss, not a false positive; (2)
  // identical signatures on both sides are never flagged -- no behavioral
  // surprise, out of scope by design.

  /// @brief opt-in per-member-name detector, invoked once per hazardous
  /// name -- same void_t presence-detection shape as HasRules/HasResult.
  #define HAPI_DETECT_MEMBER(name) \
    struct HapiMember_##name { \
      template<typename T, typename = void> \
      struct Has : std::false_type {}; \
      template<typename T> \
      struct Has<T, std::void_t<decltype(&T::name)>> : std::true_type {}; \
      template<typename T> using Sig = decltype(&T::name); \
    }

  /// @brief a component in the "O-position" of a Chain (has its own nested
  /// Part<T>, e.g. BLDCDriver3PWM) contributes members via
  /// O::template Part<Nil>; a terminal/API type (no nested Part<T>, e.g.
  /// SensorAPI) contributes directly.
  template<typename O, typename = void>
  struct HasPart : std::false_type {};
  template<typename O>
  struct HasPart<O, std::void_t<typename O::template Part<Nil>>> : std::true_type {};

  template<typename O, bool = HasPart<O>::value>
  struct MemberScope { using Type = O; };
  template<typename O>
  struct MemberScope<O, true> { using Type = typename O::template Part<Nil>; };

  /// @brief fires a legible static_assert naming Detector/A/B directly in
  /// the compiler's "required from" backtrace instead of generic template
  /// noise. The condition is template-parameter-dependent (never literally
  /// `false`), so it only fires once this exact specialization is
  /// instantiated -- the standard "dependent false" idiom.
  template<typename Detector, typename A, typename B, bool Collide>
  struct MemberCollision : std::true_type {};
  template<typename Detector, typename A, typename B>
  struct MemberCollision<Detector, A, B, true> {
    static_assert(!sizeof(Detector*),
      "HAPI: member collision -- two composed types provide the same "
      "member with different signatures, so one silently hides the other "
      "via ordinary C++ name lookup. See this MemberCollision<Detector,A,B> "
      "instantiation for which member (Detector) and which two types.");
    static constexpr bool value = false;
  };

  template<typename Detector, typename A, typename B, bool BothPresent>
  struct SigDiffers : std::false_type {};
  template<typename Detector, typename A, typename B>
  struct SigDiffers<Detector,A,B,true> : std::bool_constant<
    !std::is_same<typename Detector::template Sig<A>, typename Detector::template Sig<B>>::value> {};

  template<typename Detector, typename Elem, typename Rest> struct NoCollisionWith_;
  template<typename Detector, typename Elem>
  struct NoCollisionWith_<Detector, Elem, Chain<>> : std::true_type {};
  template<typename Detector, typename Elem, typename O, typename... OO>
  struct NoCollisionWith_<Detector, Elem, Chain<O,OO...>> {
    using SA = typename MemberScope<Elem>::Type;
    using SB = typename MemberScope<O>::Type;
    static constexpr bool bothPresent =
      Detector::template Has<SA>::value && Detector::template Has<SB>::value;
    static constexpr bool ok = MemberCollision<Detector, Elem, O,
      SigDiffers<Detector,SA,SB,bothPresent>::value>::value;
    static constexpr bool value = ok && NoCollisionWith_<Detector, Elem, Chain<OO...>>::value;
  };

  // same in-place splice as BuildRules: a head that is a container with `validates` (nested
  // Chain, nested APIOf, ...) is replaced by its children before the walk continues.
  template<typename Detector, typename Input, bool = HeadValidates<Input>::value>
  struct NoCollision_;
  template<typename Detector>
  struct NoCollision_<Detector, Chain<>, false> : std::true_type {};
  template<typename Detector, typename O, typename... OO>
  struct NoCollision_<Detector, Chain<O,OO...>, false> {
    static constexpr bool value =
      NoCollisionWith_<Detector, O, Chain<OO...>>::value &&
      NoCollision_<Detector, Chain<OO...>>::value;
  };
  template<typename Detector, typename Input>
  struct NoCollision_<Detector, Input, true>
    : NoCollision_<Detector, typename ConcatChains<typename Expand<typename Input::Head>::Children,
                                                   typename Input::Tail>::Type> {};

  /// @brief public entry point, same calling convention as Requires/
  /// Excludes above (direct bool, no ::value) -- usable standalone in a
  /// static_assert at any Chain<> composition site (bare or via APIOf),
  /// or from inside a component's own rules<Before,After>() for APIOf-
  /// based compositions that want it folded in automatically (reconstruct
  /// the full list first via ConcatChains<Before,Chain<Self>,After>).
  template<typename Detector, typename Input>
  inline constexpr bool NoCollision = NoCollision_<Detector, Input>::value;

  // ====================== DISTINCT LAYERS ======================--
  // No layer may occur twice in one composition. Checked on exact types at instantiation, so it sees what a
  // source-level check cannot: pack elements, aliases (Wave<...> vs WaveOf<Slot<...>,...>), equal types spelled
  // differently (Bias<1> vs Bias<0+1>), and types defined in other headers. The list is flattened first:
  //   Chain<...>              spliced: a nested chain is its elements
  //   a type with ::Types     a named composition (a struct over a chain, an APIOf): replaced by its Types, recursively
  //   a type with Part<O>     an open layer: compared by is_same
  //   anything else           a closed operand: compared by is_same, and by is_base_of (either way) with the other
  //                           closed operands, so a closed type and one derived from it do not both appear
  // Usage: static_assert(hapi::Distinct<Chain<A,B,OO...,T>>, "duplicate layer in Z");

  template<typename O, typename = void> struct HasTypes : std::false_type {};
  template<typename O> struct HasTypes<O, std::void_t<typename O::Types>> : std::true_type {};

  template<typename O> struct OpenLayer   { using Type = O; };
  template<typename O> struct ClosedLayer { using Type = O; };

  // 0 open layer, 1 named composition (splice its Types), 2 closed operand, 3 Chain (splice)
  template<typename O> struct LayerKind { static constexpr int value = HasTypes<O>::value ? 1 : HasPart<O>::value ? 0 : 2; };
  template<typename... OO> struct LayerKind<Chain<OO...>> { static constexpr int value = 3; };

  template<typename O, int = LayerKind<O>::value> struct LayersOf_;
  template<typename O> struct LayersOf_<O,0> { using Type = Chain<OpenLayer<O>>; };
  template<typename O> struct LayersOf_<O,1> { using Type = typename LayersOf_<typename O::Types>::Type; };
  template<typename O> struct LayersOf_<O,2> { using Type = Chain<ClosedLayer<O>>; };
  template<typename... OO> struct LayersOf_<Chain<OO...>,3> {
    using Type = typename ConcatChains<typename LayersOf_<OO>::Type...>::Type;
  };
  /// @brief the flattened layer list Distinct compares: Chain<OpenLayer<X>|ClosedLayer<X>...>
  template<typename L> using LayersOf = typename LayersOf_<L>::Type;

  template<typename A, typename B> struct LayerClash : std::false_type {};
  template<typename A, typename B> struct LayerClash<OpenLayer<A>, OpenLayer<B>> : std::is_same<A,B> {};
  template<typename A, typename B> struct LayerClash<ClosedLayer<A>, ClosedLayer<B>>
    : std::bool_constant<std::is_same<A,B>::value || std::is_base_of<A,B>::value || std::is_base_of<B,A>::value> {};

  template<typename E, typename L> struct ClashesWith;
  template<typename E> struct ClashesWith<E, Chain<>> : std::false_type {};
  template<typename E, typename O, typename... OO> struct ClashesWith<E, Chain<O,OO...>>
    : std::bool_constant<LayerClash<E,O>::value || ClashesWith<E, Chain<OO...>>::value> {};

  template<typename L> struct Distinct_;
  template<> struct Distinct_<Chain<>> : std::true_type {};
  template<typename O, typename... OO> struct Distinct_<Chain<O,OO...>>
    : std::bool_constant<!ClashesWith<O, Chain<OO...>>::value && Distinct_<Chain<OO...>>::value> {};

  /// @brief true when no layer of the (flattened) list L occurs twice; same calling convention as Requires/Excludes/NoCollision
  template<typename L>
  inline constexpr bool Distinct = Distinct_<LayersOf<L>>::value;

};
// ---- end hapi/rules.h ----
// #include "hapi/meta.h" -- inlined above

namespace hapi {
  // ====================== APIOf ======================--

  /// @brief closes chain composition with a fallback API, collapsing the chain into a
  /// single C++ class inheritance that ultimately derives from the given API.
  template<typename API, typename... OO>
  struct APIOf : Chain<OO...>::template Part<API> {
    using Base = typename Chain<OO...>::template Part<API>;
    using Base::Base;
    using Types=Chain<API,OO...>;

    /// @brief expose Part<T> for runtime find<Q> operations
    template<typename T>
    using Part = typename Chain<OO...>::template Part<T>;

    // validated over Types (API included), not just OO... — API is a real
    // component in the resulting inheritance chain and must be visible to
    // ordering/uniqueness rules like any other element.
    static_assert(BuildRules<Chain<>,Chain<API,OO...>>::rules(), "HAPI: validation failed");
  };

  /// @brief an APIOf holds its API first, then its components: exactly Types, and
  /// what BuildRules/NoCollision walk when they splice a nested APIOf. Only `validates`
  /// is on: today those two splice a nested APIOf, while Traverse-based queries,
  /// Filter/Map, and FindFirst treat it as a leaf (whole element).
  template<typename API, typename... OO>
  struct Expand<APIOf<API,OO...>> : Expansion<Chain<API,OO...>,false,false,true,false> {};

  /// @brief an APIOf has no rules() of its own (its components' rules are what BuildRules walks), so the splice must not
  /// probe it: that would instantiate the whole composed class. See HasOwnRules in rules.h.
  template<typename API, typename... OO>
  struct HasOwnRules<APIOf<API,OO...>> : std::false_type {};

}; // namespace hapi
// ---- end hapi/hapi.h ----
