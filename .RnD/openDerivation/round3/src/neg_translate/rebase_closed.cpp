// expect: [od-rule8] 'K' already has a base
struct Y {}; struct E {};
struct K : Y {};
struct Z : K:E {};
