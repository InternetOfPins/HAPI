// hapi/rules.h
#pragma once

namespace hapi {

  // Put this in your core (Chain or a traits file)
  // template<typename Comp, typename Tag>
  // struct Has {
  //     static constexpr bool value = false;
  // };

  #define REQUIRE_FEATURE(Comp, Feature) \
    static_assert(Has<Comp, Feature>::value, "Missing required feature: " #Feature)

  #define INCOMPATIBLE_WITH(Comp, Feature) \
    static_assert(!Has<Comp, Feature>::value, "Incompatible feature: " #Feature)
    
  // Very lightweight AndAll - works since C++11
  template<bool... Bs>
  struct AndAll {
    static constexpr bool value = true;
  };

  template<bool Head, bool... Tail>
  struct AndAll<Head, Tail...> {
    static constexpr bool value = Head && AndAll<Tail...>::value;
  };

  // ====================== VALIDATION ENGINE ======================

  // Primary template - features will specialize this
  template<typename Comp>
  struct CheckRules {
    static constexpr bool value = true;   // default = ok
  };

  // Top-level validation runner
  template<typename Composition>
  struct Validate {
    static constexpr bool run() {
      return CheckRules<Composition>::value;
    }
  };

};//namespace hapi