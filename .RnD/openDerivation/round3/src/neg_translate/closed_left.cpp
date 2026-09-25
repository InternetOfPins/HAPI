// expect: [od-rule7] 'K' is closed
struct K {int k=0;};
struct E {};
struct Z : K:final E {};
