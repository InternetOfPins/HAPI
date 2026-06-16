/**
 * @file avr_std.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief AVR helper file, patching C++17 for AVRs
 * sourcing from GNU ISO C++ Library with minimal changes.
 */

#pragma once

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

  using size_t = unsigned int;

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
  constexpr _Tp&& forward(typename remove_reference<_Tp>::type& __t) noexcept { 
    return static_cast<_Tp&&>(__t); 
  }

  template<typename _Tp>
  constexpr _Tp&& forward(typename remove_reference<_Tp>::type&& __t) noexcept {
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
  const char* bitset(const T val) {
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

  template<class T, T = T{0} + T{0}> struct is_integral : true_type {};

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