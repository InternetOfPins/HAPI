// expect: [od-rule9] a pack fold over ':' must be parenthesized
struct T {};
template<typename... PP> struct F : PP : ... : T {};
