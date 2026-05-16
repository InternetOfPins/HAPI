/**
 * @file xtensa_std.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief patching C++17 for xtensa (ESPs)
 * @version 5
 * @date 2026-05-14
 * 
 * @copyright Copyright (c) 2026
 * 
*/

namespace std {
  template<bool _Cond, typename _Tp = void>
  using enable_if_t = typename enable_if<_Cond, _Tp>::type;

  template< class T, class U >
  constexpr const bool is_same_v = is_same<T, U>::value;
};