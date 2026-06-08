# HAPI

## Disabling injection

**this will not work**

```c++
#pragma once

namespace hapi {
namespace detail {

  // Pure, raw linear builder bypassing all injection logic
  template <typename Base, typename... Modules>
  struct FoldLinear;

  template <typename Base>
  struct FoldLinear<Base> {
    using Type = Base;
  };

  template <typename Base, typename Head, typename... Tail>
  struct FoldLinear<Base, Head, Tail...> {
    struct Type : Head {
      using CurrentBase = typename FoldLinear<Base, Tail...>::Type;
      using Head::Head; // Pull constructors
    };
  };

} // namespace detail

  // Main Chain layout template
  template <typename... OO>
  struct Chain {
    
    // Safety rules remain active across both modes
    static constexpr bool rules() { 
      return BuildRules<Chain<>, Chain<OO...>>::rules(); 
    }

    template <typename API>
    struct Part {
#ifdef HAPI_NO_INJECTION
      // Restricted Env: Brute force linear composition
      using Type = typename detail::FoldLinear<API, OO...>::Type;
#else
      // Standard Env: Default pipeline evaluating Ins/App rules
      using Type = typename detail::InjectDependencies<API, OO...>::Type;
#endif
    };
  };

  // Main API definition mechanism
  template<typename API, typename... OO>
  struct APIOf : Chain<OO...>::template Part<API>::Type {
    using Base = typename Chain<OO...>::template Part<API>::Type;
    using Base::Base;
    
    static_assert(Chain<OO...>::rules(), "HAPI: validation failed");
  };

} // namespace hapi
```
