// expect: [od-rule8] 'K' already has a base
struct Y {}; struct E {};
struct K : Y {};
using Z = K:E;
