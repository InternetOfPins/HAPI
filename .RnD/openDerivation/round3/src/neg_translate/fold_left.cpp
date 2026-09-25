// expect: [od-rule9] only the right fold
struct T {};
template<typename... PP> struct F : (T : ... : PP) {};
