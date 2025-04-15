#pragma once

//AVR helper file
//sourcing from GNU ISO C++ Library.
// with minimal changes to make it compile on AVR's
// we also took only the needed parts as to convert all the library would be overwhelming.

#ifdef max
  #undef max
#endif
#ifdef min
  #undef min
#endif
#ifdef abs
  #undef abs
#endif
template<typename O> O min(O a,O b) {return a<b?a:b;}
template<typename O> O max(O a,O b) {return a>b?a:b;}
template<typename X> X abs(X x) {return x<0?-x:x;}

// #include <stddef.h>     // for size_t
// #include <limits>
// #include <type_traits.h>

namespace avr_std {
  template<typename T> struct numeric_limits {
    static constexpr T max() noexcept {return ~min();}
    static constexpr T min() noexcept {return ((T)0)-((T)1)>0?0:(~0)<<((sizeof(T)<<3)-1);}
  };
  // template<> struct numeric_limits<unsigned int> {
  //   static constexpr unsigned int max() noexcept {return ~min();}
  //   static constexpr unsigned int min() noexcept {return 0;}
  // };
  // template<> struct numeric_limits<unsigned char> {
  //   static constexpr unsigned char max() noexcept {return ~min();}
  //   static constexpr unsigned int min() noexcept {return 0;}
  // };
  // template<> struct numeric_limits<unsigned long> {
  //   static constexpr unsigned long max() noexcept {return min();}
  //   static constexpr unsigned int min() noexcept {return 0;}
  // };
  
  // from move.h ------------------------------------------------------------------------
  /// integral_constant
  template<typename _Tp, _Tp __v>
    struct integral_constant
    {
      static constexpr _Tp                  value = __v;
      typedef _Tp                           value_type;
      typedef integral_constant<_Tp, __v>   type;
      constexpr operator value_type() const noexcept { return value; }
#if __cplusplus > 201103L


// #define __cpp_lib_integral_constant_callable 201304

      constexpr value_type operator()() const noexcept { return value; }
#endif
    };

  template<typename _Tp, _Tp __v>
    constexpr _Tp integral_constant<_Tp, __v>::value;

  /// The type used as a compile-time boolean with true value.
  using true_type =  integral_constant<bool, true>;

  /// The type used as a compile-time boolean with false value.
  using false_type = integral_constant<bool, false>;

  /// @cond undocumented
  /// bool_constant for C++11
  template<bool __v>
    using __bool_constant = integral_constant<bool, __v>;
  /// @endcond

  // Const-volatile modifications.

  /// remove_const
  template<typename _Tp> struct remove_const { using type=_Tp; };
  template<typename _Tp> struct remove_const<_Tp const> {using type=_Tp;};
  /// remove_volatile
  template<typename _Tp> struct remove_volatile {using type=_Tp;};
  template<typename _Tp> struct remove_volatile<_Tp volatile> {using type=_Tp;};
  /// remove_cv
  template<typename _Tp> struct remove_cv {using type = _Tp;};
  template<typename _Tp> struct remove_cv<const _Tp> {using type=_Tp;};
  template<typename _Tp> struct remove_cv<volatile _Tp> {using type=_Tp;};
  template<typename _Tp> struct remove_cv<const volatile _Tp> {using type=_Tp;};
  /// add_const
  template<typename _Tp> struct add_const {using type=_Tp const;};
  /// add_volatile
  template<typename _Tp> struct add_volatile {using type=_Tp volatile;};
  /// add_cv
  template<typename _Tp> struct add_cv {using type=typename add_const<typename add_volatile<_Tp>::type>::type;};
  /// is_lvalue_reference
  template<typename> struct is_lvalue_reference: false_type {};
  template<typename _Tp> struct is_lvalue_reference<_Tp&>:true_type {};

  /// remove_reference
  template<typename _Tp> struct remove_reference {using type=_Tp;};
  template<typename _Tp> struct remove_reference<_Tp&> {using type=_Tp;};
  template<typename _Tp> struct remove_reference<_Tp&&> {using type=_Tp;};

  template<typename _Tp>
    constexpr _Tp&&
    forward(typename remove_reference<_Tp>::type& __t) noexcept
    { return static_cast<_Tp&&>(__t); }

  /**
   *  @brief  Forward an rvalue.
   *  @return The parameter cast to the specified type.
   *
   *  This function is used to implement "perfect forwarding".
   */
  template<typename _Tp>
    constexpr _Tp&&
    forward(typename remove_reference<_Tp>::type&& __t) noexcept
    {
      static_assert(!is_lvalue_reference<_Tp>::value,
    "forward must not be used to convert an rvalue to an lvalue");
      return static_cast<_Tp&&>(__t);
    }

  // Macro to warn about unused results.
  #if __cplusplus >= 201703L
  # define _GLIBCXX_NODISCARD [[__nodiscard__]]
  #else
  # define _GLIBCXX_NODISCARD
  #endif

  template<typename _Tp>
    _GLIBCXX_NODISCARD
    constexpr typename remove_reference<_Tp>::type&&
    move(_Tp&& __t) noexcept
    { return static_cast<typename remove_reference<_Tp>::type&&>(__t); }

  // Metaprogramming helper types.

  template<bool, typename, typename>  struct conditional;

  // /// @cond undocumented
  // template <typename _Type>
  //   struct __type_identity
  //   { using type = _Type; };

  // template<typename _Tp>
  //   using __type_identity_t = typename __type_identity<_Tp>::type;

  // Primary template.
  /// Define a member typedef @c type to one of two argument types.
  template<bool _Cond, typename _Iftrue, typename _Iffalse>
    struct conditional
    { typedef _Iftrue type; };

  // Partial specialization for false.
  template<typename _Iftrue, typename _Iffalse>
    struct conditional<false, _Iftrue, _Iffalse>
    { typedef _Iffalse type; };

  // template<typename...> struct __or_;
  // template<> struct __or_<>:false_type {};

  // template<typename... OO> struct __or_:__bool_constant<false((||OO::type)...)> {};
  // template<typename... OO> struct __and_:__bool_constant<true(&&OO::type)...> {};
  // template<typename _Pp> struct __not_: __bool_constant<!bool(_Pp::value)> {};

  template<typename...> using void_t = void;

  // template<typename _Tp> 
  // struct __success_type {using type=_Tp;};
  // struct __failure_type {};

  // __remove_cv_t (remove_cv_t for C++11).
  // template<typename _Tp>
  //   using __remove_cv_t = typename remove_cv<_Tp>::type;

  // Primary template.
  /// Define a member typedef `type` only if a boolean constant is true.
  template<bool, typename _Tp = void>
    struct enable_if
    { };

  // Partial specialization for true.
  template<typename _Tp>
    struct enable_if<true, _Tp>
    { typedef _Tp type; };

  /// @cond undocumented

  // __enable_if_t (enable_if_t for C++11)
  template<bool _Cond, typename _Tp = void>
    using __enable_if_t = typename enable_if<_Cond, _Tp>::type;

  template<class T>
  struct is_pointer : false_type {};
  
  template<class T> struct is_pointer<T*> : true_type {};
  template<class T> struct is_pointer<T* const> : true_type {};
  template<class T> struct is_pointer<T* volatile> : true_type {};
  template<class T> struct is_pointer<T* const volatile> : true_type {};

  template< class T > struct remove_pointer                    {typedef T type;};
  template< class T > struct remove_pointer<T*>                {typedef T type;};
  template< class T > struct remove_pointer<T* const>          {typedef T type;};
  template< class T > struct remove_pointer<T* volatile>       {typedef T type;};
  template< class T > struct remove_pointer<T* const volatile> {typedef T type;};

  namespace detail {
  
  template <class T>
  struct type_identity { using type = T; }; // or use type_identity (since C++20)
  
  template <class T> // Note that `cv void&` is a substitution failure
  auto try_add_lvalue_reference(int) -> type_identity<T&>;
  template <class T> // Handle T = cv void case
  auto try_add_lvalue_reference(...) -> type_identity<T>;
  
  template <class T>
  auto try_add_rvalue_reference(int) -> type_identity<T&&>;
  template <class T>
  auto try_add_rvalue_reference(...) -> type_identity<T>;
  
  } // namespace detail
  
  template <class T>
  struct add_lvalue_reference : decltype(detail::try_add_lvalue_reference<T>(0)) {};
  
  template <class T>
  struct add_rvalue_reference : decltype(detail::try_add_rvalue_reference<T>(0)) {};

  // template<typename T> constexpr bool always_false = false;
  
  template<typename T>
  typename add_rvalue_reference<T>::type declval() noexcept {
      // static_assert(always_false<T>, "declval not allowed in an evaluated context");
  }

  template<class T, class U>
  struct is_same : false_type {};
  
  template<class T>
  struct is_same<T, T> : true_type {};

  template<typename _Tp>
    struct is_union
    : public integral_constant<bool, __is_union(_Tp)>
    { };

  template<typename _Tp>
    struct is_class
    : public integral_constant<bool, __is_class(_Tp)>
    { };

  // __remove_cv_t (remove_cv_t for C++11).
  template<typename _Tp>
    using __remove_cv_t = typename remove_cv<_Tp>::type;

  /// is_const
  template<typename>
    struct is_const
    : public false_type { };

  template<typename _Tp>
    struct is_const<_Tp const>
    : public true_type { };

  /// is_function
  template<typename _Tp>
    struct is_function
    : public __bool_constant<!is_const<const _Tp>::value> { };

  template<typename _Tp>
    struct is_function<_Tp&>
    : public false_type { };

  template<typename _Tp>
    struct is_function<_Tp&&>
    : public false_type { };

  template<typename>
    struct __is_member_function_pointer_helper
    : public false_type { };

  template<typename _Tp, typename _Cp>
    struct __is_member_function_pointer_helper<_Tp _Cp::*>
    : public is_function<_Tp>::type { };

  /// is_member_function_pointer
  template<typename _Tp>
    struct is_member_function_pointer
    : public __is_member_function_pointer_helper<__remove_cv_t<_Tp>>::type
    { };

  template <class, class T, class... Args>
  struct is_constructible_ : false_type {};

  template <class T, class... Args>
  struct is_constructible_<
      void_t<decltype(T(declval<Args>()...))>,
  T, Args...> : true_type {};

  template <class T, class... Args>
  using is_constructible = is_constructible_<void_t<>, T, Args...>;

  template<int n,typename T>
  const char* bitset(const T val) {
    static char o[n+1]{0};
    for(int i=0;i<n;i++) o[n-i-1]=(val&(1<<i))?'1':'0';
    return o;
  }

  namespace details
  {
      template<typename B>
      true_type test_ptr_conv(const volatile B*);
      template<typename>
      false_type test_ptr_conv(const volatile void*);
  
      template<typename B, typename D>
      auto test_is_base_of(int) -> decltype(test_ptr_conv<B>(static_cast<D*>(nullptr)));
      template<typename, typename>
      auto test_is_base_of(...) -> true_type; // private or ambiguous base
  }
  
  template<typename Base, typename Derived>
  struct is_base_of :
      integral_constant<
          bool,
          is_class<Base>::value &&
          is_class<Derived>::value &&
          decltype(details::test_is_base_of<Base, Derived>(0))::value
      > {};

  template<class T>
  struct is_void : is_same<void, typename remove_cv<T>::type> {};

  namespace detail
  {
      template<class T>
      auto test_returnable(int) -> decltype(
          void(static_cast<T(*)()>(nullptr)), true_type{}
      );
      template<class>
      auto test_returnable(...) -> false_type;
  
      template<class From, class To>
      auto test_implicitly_convertible(int) -> decltype(
          void(declval<void(&)(To)>()(declval<From>())), true_type{}
      );
      template<class, class>
      auto test_implicitly_convertible(...) -> false_type;
  } // namespace detail
  
  template<class From, class To>
  struct is_convertible : integral_constant<bool,
      (decltype(detail::test_returnable<To>(0))::value &&
      decltype(detail::test_implicitly_convertible<From, To>(0))::value) ||
      (is_void<From>::value && is_void<To>::value)
  > {};

};
